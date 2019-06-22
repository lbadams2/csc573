#include <string>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <unistd.h>

#define PORT 7735
#define VALID_CHECKSUM 65535
#define HEADER_SIZE 86
typedef std::string string;
typedef std::unordered_map<std::string, std::string> umap;

struct ACK_Segment {
    unsigned int seq_num;
    static char zeroes[2];
    static char type[2];
    static void init_static();
};

class Receiver {
public:
    umap read_segment(string segment, bool set_mss);
    void download_file();
    void send_ack(umap &seg_map);
private:
    bool validate_checksum(uint16_t checksum, umap &seg_map);
    void set_mss(string data);
    string file_contents;
    string file_name;
    uint16_t next_seq_num;
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

static inline std::vector<bool> int_to_bool(int x) {
  std::vector<bool> ret;
  while(x) {
    if (x&1)
      ret.push_back(true);
    else
      ret.push_back(false);
    x>>=1;  
  }
  std::reverse(ret.begin(),ret.end());
  return ret;
}