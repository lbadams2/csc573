#include "Sender.h"

Sender::Sender(std::vector<string> theHosts, int thePort, string name, int theMss) : hosts(theHosts), port(thePort), file_name(name), mss(theMss) {
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

Segment Sender::create_segment(std::vector<char> data) {

}

bool Sender::read_response(int seq_num, string response) {

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
    while(file_pos < file_size - 1) {
        std::unique_lock<std::mutex> lock(mrun);
        main_ready.wait(lock, [&next_iteration, this]{return next_iteration == current_iteration; });
        lock.unlock();
        ++next_iteration;

        std::vector<char> file_chunk;
        if(file_pos + mss < file_size)
            std::vector<char> file_chunk(buffer.begin() + file_pos, buffer.begin() + file_pos + mss);
        else
            std::vector<char> file_chunk(buffer.begin() + file_pos, buffer.end());
        Segment segment = create_segment(file_chunk);

        bool is_ack = false;
        while(!is_ack) {
            if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                std::cout << "\n Connection failed \n";
                exit(EXIT_FAILURE);
            }
            string req_str = segment.to_string();
            const char* req = req_str.c_str();
            send(sock, req, strlen(req), 0);

            bzero(res_buf, length);
            int block_sz = 0;
            std::string res = "";
            while((block_sz = read(sock, res_buf, length)) > 0) {
                std::string chunk(res_buf);
                res += chunk;
                bzero(res_buf, length);
                if(block_sz == 0 || block_sz != length)
                    break;
            }
            is_ack = read_response(segment.seq_num, res);
        }
        lock.lock();
        if(--worker_count == 0) {
            lock.unlock();
            iteration_complete.notify_one();
        }
    }
    files_sent.push_back(1);
}

string Segment::to_string() const {
    return "";
}