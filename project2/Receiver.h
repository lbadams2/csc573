#include <string>
#include <utility>

#define PORT 7735;
typedef std::string string;

struct ACK_Segment {

};

class Receiver {
public:
    std::pair<bool, int> read_segment();
    void listen();
    void send_ack(int seq_num);
private:
    bool validate_checksum(int checksum);
    string file_contents;
    string file_name;
    long next_seq_num;
    int mss;
    double loss_prob;
};