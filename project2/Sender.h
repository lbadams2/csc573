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
#include <mutex>
#include <thread>
#include <condition_variable>

typedef std::string string;

struct Segment {
    int seq_num;
    int source_port;
    int dest_port;
    char* checksum;
    char* data;
    string to_string() const;
};

class Sender {
public:
    Sender(std::vector<string> hosts, const int port, const string file_name, const int mss);
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
    const int port;
    const string file_name;
    const int mss;
    std::vector<char> buffer;
    std::vector<int> files_sent;
    Segment create_segment(std::vector<char> data);
    bool read_response(int seq_num, string response);
};

#endif /* Sender_h */