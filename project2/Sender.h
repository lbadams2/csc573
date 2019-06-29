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
#include <bitset>
#include <algorithm>

#define RECEIVER_PORT 7735
using std::string;
using std::cout;
using std::vector;
using std::to_string;

struct Segment {
    unsigned int seq_num;
    static unsigned char type[2];
    uint16_t checksum;
    std::vector<unsigned char> data;
    vector<unsigned char> to_bytes(bool mss_segment, uint16_t mss);
    void remove_nulls(vector<unsigned char>& v, bool is_set_mss);
    static void init_type();
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
    std::vector<unsigned char> buffer;
    std::vector<int> files_sent;
    //Segment create_segment(std::vector<unsigned char> &data, unsigned int seq_num);
    bool read_response(unsigned int seq_num, unsigned char* res);
    void add_nulls(unsigned char* segment);
};

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

#endif /* Sender_h */
