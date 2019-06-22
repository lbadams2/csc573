#include "Sender.h"

Sender::Sender(std::vector<string> theHosts, const uint16_t thePort, const string name, const uint16_t theMss, const long theTimeout) : hosts(theHosts), port(thePort), file_name(name), mss(theMss), timeout(theTimeout) {
    std::ifstream infile(file_name, std::ios_base::binary);
    this->buffer = std::vector<char>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
}

std::vector<int>& Sender::get_files_sent() {return files_sent;}

/*
void Sender::start() {
    for(string host: hosts) {
        std::thread t(send_file, host.c_str());
    }
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
            iteration_complete.wait(lock, [this]{return worker_count == 0; });
        }
    }
}
*/

Segment Sender::create_segment(std::vector<char> &data, const uint16_t port, unsigned int seq_num) {
    Segment segment;
    segment.dest_port = RECEIVER_PORT;
    segment.source_port = port;
    segment.length = data.size();
    segment.seq_num = seq_num;
    segment.checksum = segment.dest_port + segment.source_port + segment.length;
    uint16_t concat;
    for(auto it = data.begin(); it < data.end(); it++) {
        concat = *it;
        it++;
        if(it == data.end())
            concat = concat << 8;
        else
            concat = (concat << 8) | *it;
        segment.checksum += concat;
    }
    segment.checksum = ~segment.checksum;
    segment.data = data;
    return segment;
}

string Segment::to_string() const {
    string sn = std::to_string(seq_num);
    sn = string(5 - sn.length(), '0').append(sn);
    string sp = std::to_string(source_port);
    sp = string(5 - sp.length(), '0').append(sp);
    string l = std::to_string(length);
    l = string(4 - l.length(), '0').append(l);
    string cs = std::to_string(checksum);
    cs = string(5 - cs.length(), '0').append(cs);
    string seg_str = "Seq Num: " + sn + "\r\n"; // 16 bytes
    seg_str += "Source Port: " + sp + "\r\n"; // 20  bytes
    seg_str += "Dest Port: " + std::to_string(dest_port) + "\r\n"; // 17  bytes
    seg_str += "Length: " + l + "\r\n"; // 14 bytes
    seg_str += "Checksum: " + cs + "\r\n\r\n"; // 19 bytes
    string s(data.begin(), data.end());
    seg_str += s;
    return seg_str;
}

bool Sender::read_response(unsigned int seq_num, string response) {

}

void Sender::send_file(const char* host) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    int length = 1024;
    char res_buf[1024] = {0};
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        exit(EXIT_FAILURE);
    }
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    // copy 0 into serv_addr members
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        exit(EXIT_FAILURE);
    }

    size_t file_size = buffer.size();
    size_t file_pos = 0;
    int next_iteration = 1;
    bool establish = true;
    while(file_pos < file_size - 1) {
        std::unique_lock<std::mutex> lock(mrun);
        main_ready.wait(lock, [&next_iteration, this]{return next_iteration == current_iteration; });
        lock.unlock();
        ++next_iteration;
        Segment segment;
        if(!establish) {
            std::vector<char> file_chunk;
            if(file_pos + mss < file_size) {
                std::vector<char> file_chunk(buffer.begin() + file_pos, buffer.begin() + file_pos + mss);
                file_pos += mss;
            }
            else {
                std::vector<char> file_chunk(buffer.begin() + file_pos, buffer.end());
                file_pos = file_size;
            }
            segment = create_segment(file_chunk, port, file_pos - mss);
        }
        else {
            string data_str = "MSS: " + std::to_string(mss);
            std::vector<char> mss_data(data_str.begin(), data_str.end());
            segment = create_segment(mss_data, port, 0);
        }

        bool is_ack = false, timed_out = false;
        std::chrono::high_resolution_clock::time_point start_time, end_time;
        long duration = 0;
        while(!is_ack) {
            if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                std::cout << "\n Connection failed \n";
                exit(EXIT_FAILURE);
            }
            string req_str = segment.to_string();
            const char* req = req_str.c_str();
            send(sock, req, strlen(req), 0);
            start_time = std::chrono::high_resolution_clock::now();
            bzero(res_buf, length);
            int block_sz = 0;
            std::string res = "";
            while((block_sz = read(sock, res_buf, length)) > 0) {
                std::string chunk(res_buf);
                res += chunk;
                bzero(res_buf, length);
                if(block_sz == 0 || block_sz != length)
                    break;
                end_time = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                if(duration > timeout) {
                    timed_out = true;
                    break;
                }
            }            
            if(timed_out || errno == ETIMEDOUT || errno == EWOULDBLOCK) {
                std::cout << "time out occurred on read\n";
                continue;
            }
            is_ack = read_response(segment.seq_num, res);            
        }
        lock.lock();
        if(--worker_count == 0) {
            lock.unlock();
            iteration_complete.notify_one();
        }
    }
    close(sock);
    files_sent.push_back(1);
}