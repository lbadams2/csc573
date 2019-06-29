#include <string>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <random>
#include <bitset>
#include <algorithm>
#include <cstring>

#define PORT 7735
#define VALID_CHECKSUM 65535
#define HEADER_SIZE 8
using std::cout;
using std::string;
using std::vector;
using std::to_string;
typedef std::unordered_map<std::string, std::string> umap;

struct ACK_Segment {
    unsigned char seq_num[4];
    static unsigned char zeroes[2];
    static unsigned char type[2];
    static void init_static();
};

class Receiver {
public:
    Receiver(string file_name, double loss_prob);
    umap read_segment(vector<unsigned char>& segment, ssize_t num_bytes, bool is_set_mss);
    void download_file();
    vector<unsigned char> get_ack();
private:
    bool validate_checksum(vector<unsigned char>& segment, ssize_t num_bytes);
    void add_nulls(unsigned char* segment, bool is_set_mss);
    void remove_nulls(vector<unsigned char>& v);
    string file_contents;
    string file_name;
    unsigned int next_seq_num;
    uint16_t mss;
    double loss_prob;
};

static inline unsigned char to_byte(bool b[8])
{
    unsigned char c = 0;
    for (int i=0; i < 8; ++i)
        if (b[i])
            c |= 1 << i;
    return c;
}

static inline void from_byte(unsigned char c, bool b[8])
{
    for (int i=0; i < 8; ++i)
        b[i] = (c & (1<<i)) != 0;
}

/*
 static inline bool* int_to_bool(int x, bool ret[32]) {
 bool* ret = new bool[32];
 int  i = 0;
 while(x) {
 if (x&1)
 ret[i] = 1;
 else
 ret[i] = 0;
 x>>=1;
 }
 std::reverse(std::begin(ret),std::end(ret));
 return ret;
 }
 */

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
