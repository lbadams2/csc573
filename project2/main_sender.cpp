#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <condition_variable>
#include <bitset>
#include <algorithm>

#define RECEIVER_PORT 7735
#define TIMEOUT 80000
using std::string;
using std::cout;
using std::vector;
using std::to_string;

std::mutex mrun;
int current_iteration = 0;
vector<unsigned char> buffer;
std::condition_variable main_ready;
std::condition_variable iteration_complete;
int mss = 0;
int worker_count = 0;
vector<int> files_sent;

static inline int bitArrayToInt32(bool arr[], int count)
{
    int ret = 0;
    int tmp;
    for (int i = 0; i < count; i++) {
        tmp = arr[i];
        ret |= tmp << (count - i - 1);
    }
    return ret;
}

static inline unsigned char to_byte(bool b[8])
{
    unsigned char c = 0;
    for (int i=0; i < 8; ++i)
        if (b[i])
            c |= 1 << i;
    return c;
}

struct Segment {
    unsigned int seq_num;
    static unsigned char type[2];
    uint16_t checksum;
    std::vector<unsigned char> data;
    vector<unsigned char> to_bytes(bool mss_segment, uint16_t mss);
    void remove_nulls(vector<unsigned char>& v, bool is_set_mss);
    static void init_type();
};
unsigned char Segment::type[2];

void Segment::init_type() {
    bool b[8] = {false};
    for(int i = 0; i < 8; i++)
        if(i % 2 == 0)
            b[i] = true;
        else
            b[i] = false;
    unsigned char t = to_byte(b);
    type[0] = t;
    type[1] = t;
}

vector<unsigned char> Segment::to_bytes(bool mss_segment, uint16_t mss) {
    // convert seq num to bool
    bool snb[32] = {false};
    int  i = 0;
    unsigned int sn = seq_num;
    while(sn) {
        if (sn&1)
            snb[i] = 1;
        else
            snb[i] = 0;
        sn>>=1;
        i++;
    }
    std::reverse(std::begin(snb),std::end(snb));
    vector<unsigned char> header(8);
    bool bool_byte[8];
    unsigned char seg_byte;
    int byte_num = 0;
    for(int i = 0; i < 32; i++) {
        bool_byte[i % 8] = snb[i];
        if(i % 8 == 7) {
            seg_byte = to_byte(bool_byte);
            //if(seg_byte == '\0')
            //    seg_byte = '0';
            byte_num = (i + 1)/8;
            header[byte_num - 1] = seg_byte;
        }
    }
    header[6] = type[0];
    header[7] = type[1];
    
    uint16_t concat = 0;
    uint16_t sum = 0;
    for(int i = 0; i < 4; i += 2) {
        concat = header[i];
        concat = concat << 8;
        concat = concat | header[i + 1];
        sum += concat;
    }
    //cout << "seg sum " << to_string(sum) << "\n";
    concat = header[6];
    concat = concat << 8;
    concat = concat | header[7];
    sum += concat;
    //cout << "sum with type " << to_string(sum) << "\n";
    if(mss_segment) {
        vector<unsigned char> mss_data(2);
        bool mssb[16] = {false};
        i = 0;
        uint16_t m = mss;
        while(m) {
            if (m&1)
                mssb[i] = 1;
            else
                mssb[i] = 0;
            m>>=1;
            i++;
        }
        std::reverse(std::begin(mssb),std::end(mssb));
        byte_num = 0;
        unsigned char mssbyte;
        for(int i = 0; i < 16; i++) {
            bool_byte[i % 8] = mssb[i];
            if(i % 8 == 7) {
                mssbyte = to_byte(bool_byte);
                //if(mssbyte == '\0')
                //    mssbyte = '0';
                byte_num = (i + 1)/8 - 1;
                mss_data[byte_num] = mssbyte;
            }
        }
        data = mss_data;
    }
    for(auto it = data.begin(); it < data.end(); it++) {
        concat = *it;
        it++;
        if(it == data.end())
            concat = concat << 8;
        else
            concat = (concat << 8) | *it;
        sum += concat;
    }
    //cout << "sum with data " << to_string(sum) << "\n";
    checksum = ~sum;
    //cout << "checksum " << to_string(sum) << "\n";
    bool csb[16] = {false};
    i = 0;
    uint16_t cs = checksum;
    while(cs) {
        if (cs&1)
            csb[i] = 1;
        else
            csb[i] = 0;
        cs>>=1;
        i++;
    }
    std::reverse(std::begin(csb),std::end(csb));
    byte_num = 0;
    unsigned char csbyte;
    for(int i = 0; i < 16; i++) {
        bool_byte[i % 8] = csb[i];
        if(i % 8 == 7) {
            csbyte = to_byte(bool_byte);
            //if(csbyte == '\0')
            //    csbyte = '0';
            byte_num = (i + 1)/8 + 3;
            header[byte_num] = csbyte;
        }
    }
    
    vector<unsigned char> all;
    cout << "header size " << std::to_string(header.size()) << " data size " << std::to_string(data.size()) << "\n";
    all.reserve(header.size() + data.size());
    all.insert(all.end(), header.begin(), header.end());
    all.insert(all.end(), data.begin(), data.end());
    //remove_nulls(all, mss_segment);
    return all;
}

bool read_response(unsigned int seq_num, unsigned char* response) {
    bool all[32] = {false};
    for(int i = 0; i < 4; i++) {
        std::bitset<8> bset(response[i]);
        for(int j = i * 8; j < 8*(i+1); j++)
            all[j] = bset[j % 8];
    }
    int ans = bitArrayToInt32(all, 32);
    //cout << "bit array converted: " << std::to_string(ans) << " seq num " << std::to_string(seq_num)  <<  std::endl;
    return ans == seq_num;
}

void send_file(const char* host) {
    //cout << host << " in send file\n";
    int sock = 0;
    struct sockaddr_in serv_addr;
    size_t length = 8;
    unsigned char res_buf[8] = {0};
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        exit(EXIT_FAILURE);
    }
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    // copy 0 into serv_addr members
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(RECEIVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(host);
    /*
     if(inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
     std::cout << "\nInvalid address/ Address not supported \n";
     exit(EXIT_FAILURE);
     }
     */
    
    size_t file_size = buffer.size();
    //cout << "file size " << to_string(file_size) << "\n";
    size_t file_pos = 0;
    int next_iteration = 1;
    bool establish = true;
    //std::unique_lock<std::mutex> lock(mrun);
    while(file_pos < file_size - 1) {
        cout << host << " waiting for lock\n";
        std::unique_lock<std::mutex> lock(mrun);
        main_ready.wait(lock, [&next_iteration]{return next_iteration == current_iteration; });
        cout << host << " acquired lock\n";
        lock.unlock();
        ++next_iteration;
        Segment segment;
        vector<unsigned char> req_str;
        if(!establish) {
            if(file_pos + mss < file_size) {
                std::vector<unsigned char> file_chunk(buffer.begin() + file_pos, buffer.begin() + file_pos + mss);
                segment.data = file_chunk;
                file_pos += mss;
            }
            else {
                std::vector<unsigned char> file_chunk(buffer.begin() + file_pos, buffer.end());
                segment.data = file_chunk;
                file_pos = file_size;
            }
            segment.seq_num = file_pos - mss;
            req_str = segment.to_bytes(false, mss);
            //segment = create_segment(file_chunk, file_pos - mss);
        }
        else {
            segment.seq_num = 0;
            req_str = segment.to_bytes(true, mss);
            //segment = create_segment(mss_data, 0);
        }
        
        bool is_ack = false, timed_out = false;
        std::chrono::high_resolution_clock::time_point start_time, end_time;
        long duration = 0;
        unsigned char* req = req_str.data();
        size_t num_bytes = req_str.size();
        unsigned int len = sizeof serv_addr;
        while(!is_ack) {
            /*
             if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
             std::cout << "\n Connection failed \n";
             exit(EXIT_FAILURE);
             }
             */
            //vector<unsigned char> req_str = segment.to_bytes();
            //validate_checksum(req, req_str.size());
            //cout << host << " about to send data " << std::to_string(segment.seq_num) << " bytes " << std::to_string(num_bytes) << "\n";
            ssize_t send_res = sendto(sock, req, num_bytes, 0, (const struct sockaddr *) &serv_addr, sizeof(serv_addr));
            printf("Sent to %s:%d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
            //cout << host << " send res " << to_string(send_res) << " " << strerror(errno) << "\n";
            //send(sock, req, num_bytes, 0);
            //cout << "sent data\n";
            start_time = std::chrono::high_resolution_clock::now();
            bzero(res_buf, length);
            cout << host << " about to read ack\n";
            ssize_t block_sz = recvfrom(sock, res_buf, length, 0, (struct sockaddr *) &serv_addr, &len);
            //ssize_t block_sz = read(sock, res_buf, length);
            cout << host << " read ack\n";
            //if(block_sz == 0 || block_sz != length)
            //    break;
            end_time = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            cout << host << " block sz " << std::to_string(block_sz) << " duration " << std::to_string(duration) << std::endl;
            if(duration > TIMEOUT / 1000)
                timed_out = true;
            if(timed_out || errno == ETIMEDOUT || block_sz < 0) {
                std::cout << "time out/error occurred on read " << to_string(errno) << "\n";
                cout << strerror(errno) << "\n";
                bzero(res_buf, length);
                timed_out = false;
                continue;
            }
            //add_nulls(res_buf);
            is_ack = read_response(segment.seq_num, res_buf);
            cout << host << " is ack " << is_ack << "\n\n";
            bzero(res_buf, length);
            close(sock);
            if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
                perror("socket failed");
                exit(EXIT_FAILURE);
            }
            tv.tv_sec = 0;
            tv.tv_usec = TIMEOUT;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        }
        establish = false;
        lock.lock();
        if(--worker_count == 0) {
            lock.unlock();
            iteration_complete.notify_one();
        }
        //lock.unlock();
    }
    close(sock);
    cout << host << " Out of while loop\n";
    std::unique_lock<std::mutex> lock(mrun);
    main_ready.wait(lock, [&next_iteration]{return next_iteration == current_iteration; });
    cout << host << " acquired lock\n";
    files_sent.push_back(1);
    if(--worker_count == 0) {
        lock.unlock();
        iteration_complete.notify_one();
    }
    else
        lock.unlock();
}


int main(int argc, char** argv) {
    mss = atoi(argv[argc-1]);
    std::string file_name = argv[argc-2];
    int port = atoi(argv[argc-3]);
    std::vector<std::string> hosts;
    for(int i = 1; i < argc-3; i++)
        hosts.push_back(argv[i]);
    std::ifstream infile(file_name, std::ios_base::binary);
    buffer = std::vector<unsigned char>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
    Segment::init_type();
    
    //Sender sender(hosts, port, file_name, mss, 2);
    //std::mutex mrun;
    std::vector<std::thread> threads;
    threads.reserve(hosts.size());
    for(string host: hosts) {
        //std::thread t(&Sender::send_file, &sender, host.c_str());
        //std::thread* t = new std::thread(&Sender::send_file, &sender, host.c_str());
        //threads.push_back(t);
        threads.emplace_back(std::thread([=]{send_file(host.c_str());} ));
        //threads.push_back(std::move(t));
    }
    //std::vector<int> files_sent = sender.get_files_sent();
    std::chrono::high_resolution_clock::time_point start_time, end_time;
    start_time = std::chrono::high_resolution_clock::now();
    while(files_sent.size() < hosts.size())
    {
        {
            std::lock_guard<std::mutex> lock(mrun);
            worker_count = hosts.size();
            ++current_iteration;
        }
        main_ready.notify_all();
        {
            std::unique_lock<std::mutex> lock(mrun);
            iteration_complete.wait(lock, [&]{return ::worker_count == 0; });
        }
    }
    end_time = std::chrono::high_resolution_clock::now();
    long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    double secs = duration/(double)1000;
    cout << "elapsed time for all uploads " << to_string(secs) << "\n";
    cout << "out of main while loop\n";
    for(auto& t: threads) {
        t.join();
        //delete t;
    }    
}
