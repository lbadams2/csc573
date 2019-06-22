#ifndef Sender_h
#define Sender_h

#include <string>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>

#define RECEIVER_PORT 7735
typedef std::string string;

struct Segment {
    unsigned int seq_num;
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
    std::vector<char> data;
    string to_string() const;
};

class Sender {
public:
    Sender(std::vector<string> hosts, const uint16_t port, const string file_name, const uint16_t mss, const long timeout);
    void send_file(const char* host);
    void start();
    std::vector<int>& get_files_sent();
    int worker_count;
    int current_iteration;
    std::mutex mrun;
    std::condition_variable main_ready;
    std::condition_variable iteration_complete;
private:    
    std::vector<string> hosts;
    const uint16_t port;
    const string file_name;
    const uint16_t mss;
    const long timeout;
    std::vector<char> buffer;
    std::vector<int> files_sent;
    Segment create_segment(std::vector<char> &data, const uint16_t port, unsigned int seq_num);
    bool read_response(unsigned int seq_num, string response);
};

#endif /* Sender_h */