#include "Receiver.h"

unsigned char ACK_Segment::zeroes[2];
unsigned char ACK_Segment::type[2];

void ACK_Segment::init_static() {
    bool b[8] = {false};
    unsigned char zs = to_byte(b);
    zeroes[0] = zs;
    zeroes[1] = zs;
    for(int i = 0; i < 8; i++)
        if(i % 2 == 0)
            b[i] = false;
        else
            b[i] = true;
    unsigned char t = to_byte(b);
    type[0] = t;
    type[1] = t;
}

Receiver::Receiver(string fn, double lp): file_name(fn), loss_prob(lp), mss(0), next_seq_num(0) {}

void Receiver::add_nulls(unsigned char* segment, bool is_set_mss) {
    if(is_set_mss) {
        if(segment[8] == '0')
            segment[8] = '\0';
        if(segment[9] == '0')
            segment[9] = '\0';
    }
    for(int i = 0; i < 4; i++) {
        if(segment[i] == '0')
            segment[i] = '\0';
    }
    if(segment[4] == '0')
        segment[4] = '\0';
    if(segment[5] == '0')
        segment[5] = '\0';
}

umap Receiver::read_segment(vector<unsigned char>& segment, ssize_t num_bytes, bool is_set_mss) {
    umap map;
    //add_nulls(segment, is_set_mss);
    bool is_valid = validate_checksum(segment, num_bytes);
    if(is_valid)
        map["checksum_valid"] = "true";
    else {
        map["checksum_valid"] = "false";
        return map;
    }
    
    bool bits[32] = {false};
    for(int i = 0; i < 4; i++) {
        unsigned char c = segment[i];
        std::bitset<8> bset(c);
        for(int j = i * 8; j < 8*(i+1); j++)
            bits[j] = bset[j % 8];
    }
    unsigned int seq_num = bitArrayToInt32(bits, 32);
    cout << "Received seq num " << std::to_string(seq_num) << "\n";    
    if(seq_num == next_seq_num) {
        map["in_order"] = "true";
        next_seq_num += mss;
    }
    else if(seq_num == (next_seq_num - mss + num_bytes - HEADER_SIZE)) {
        map["in_order"] = "true";
        cout << "Last packet\n";
        next_seq_num = seq_num + mss;
    }
    else {
        cout << "Next seq num " << to_string(next_seq_num) << "\n";
        map["in_order"] = "false";
        return map;
    }
    
    if(is_set_mss) {
        bool all[16] = {false};
        std::bitset<8> bits = segment[8];
        for(int i = 0; i < 8; i++)
            all[i] = bits[i];
        bits = segment[9];
        for(int i = 8; i < 16; i++)
            all[i] = bits[i % 8];
        mss = bitArrayToInt32(all, 16);
        cout << "mss is " <<  to_string(mss) << "\n";
        next_seq_num = 0;
    }
    else {
        string data_str;
        for(int i = 8; i < num_bytes; i++) {
            char c = (char)segment[i];
            data_str += c;
        }
        map["data"] = data_str;
    }
    
    return map;
}

bool Receiver::validate_checksum(vector<unsigned char>& segment, ssize_t num_bytes) {
    bool all[16] = {false};
    std::bitset<8> bits = segment[4];
    for(int i = 0; i < 8; i++)
        all[i] = bits[i];
    bits = segment[5];
    for(int i = 8; i < 16; i++)
        all[i] = bits[i % 8];
    uint16_t checksum = bitArrayToInt32(all, 16);
    cout << "checksum " << to_string(checksum) << "\n";
    uint16_t concat = 0;
    uint16_t sum = 0;
    for(int i = 0; i < 4; i += 2) {
        //if(segment[i] == '0')
        //    segment[i] = '\0';
        concat = segment[i];
        concat = concat << 8;
        concat = concat | segment[i + 1];
        sum += concat;
    }
    cout << "seg sum " << to_string(sum) << "\n";
    concat = segment[6];
    concat = concat << 8;
    concat = concat | segment[7];
    sum += concat;
    cout << "sum with type " << to_string(sum) << "\n";
    for(int i = 8; i < num_bytes; i++) {
        concat = segment[i];
        if( ++i == num_bytes)
            concat = concat << 8;
        else
            concat = (concat << 8) | segment[i];
        sum += concat;
    }
    cout << "sum with data " << to_string(sum) << "\n";
    uint16_t val = sum + checksum;
    cout << "Sum plus checksum " << std::to_string(val) << "\n";
    return val == VALID_CHECKSUM;
}

vector<unsigned char> Receiver::get_ack() {
    // convert seq num to bool
    bool snb[32] = {false};
    int i = 0;
    unsigned int ns = 0;
    if(next_seq_num == 0)
        ns = next_seq_num;
    else
        ns = next_seq_num - mss;
    cout << "seq num in ack " << to_string(ns) << "\n";
    while(ns) {
        if (ns&1)
            snb[i] = 1;
        else
            snb[i] = 0;
        ns>>=1;
        i++;
    }
    std::reverse(std::begin(snb),std::end(snb));
    vector<unsigned char> ack_bytes(8);
    bool bool_byte[8] = {false};
    unsigned char seg_byte;
    int byte_num = 0;
    for(int i = 0; i < 32; i++) {
        bool_byte[i % 8] = snb[i];
        if(i % 8 == 7) {
            seg_byte = to_byte(bool_byte);
            byte_num = (i + 1)/8;
            ack_bytes[byte_num - 1] = seg_byte;
        }
    }
    ack_bytes[4] = ACK_Segment::zeroes[0];
    ack_bytes[5] = ACK_Segment::zeroes[1];
    ack_bytes[6] = ACK_Segment::type[0];
    ack_bytes[7] = ACK_Segment::type[1];
    return ack_bytes;
    //string ack_str = "Seq num: " + seq_bytes + "\r\n" + ACK_Segment::zeroes + "\r\n" + ACK_Segment::type + "\r\n";
}

void Receiver::remove_nulls(vector<unsigned char>& v) {
    auto it = v.begin();
    while(it != v.end()) {
        if(*it == '\0')
            *it = '0';
        it++;
    }
}

void Receiver::download_file() {
    int server_fd;
    struct sockaddr_in serv_addr, cli_addr;
    int segment_size = 20;
    umap seg_map;
    //unsigned char buffer[20] = {0};
    //vector<unsigned char> bvec(20);
    ACK_Segment::init_static();
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);
    if((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    
    // bind socket to port
    //bzero(buffer, segment_size);
    ssize_t block_sz = 0;
    unsigned int len = sizeof cli_addr;
    bool is_set_mss = true;
    if(bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //unsigned char* buffer = bvec.data();
    vector<unsigned char> bvec(segment_size);
    unsigned char* buffer = bvec.data();
    bool is_resized = false;
    bool first = true;
    while(true) {
        cout << "about to recvfrom\n";
        //unsigned char buffer[segment_size] = {0};
        //bzero(buffer, segment_size);
        while((block_sz = recvfrom(server_fd, buffer, segment_size, 0, ( struct sockaddr *) &cli_addr, &len)) > 0) {
            cout << "Received data - bytes " << std::to_string(block_sz) << "\n";
            printf("Received from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            double rand_val = dis(gen);
            if(rand_val <= loss_prob || first) {
                first = false;
                cout << "Lost packet\n";
                continue;
            }
            //string segment(buffer);
            if(is_set_mss) {
                seg_map = read_segment(bvec, block_sz, true);
                if(seg_map["checksum_valid"] == "true") {
                    segment_size = mss + HEADER_SIZE;
                    is_resized = true;
                    //buffer[segment_size] = {0};
                    //bvec.clear();
                    //bvec.resize(segment_size);
                    //buffer = bvec.data();
                }
                //is_set_mss = false;
            }
            else {
                if(block_sz == 0) {
                    cout << "received no data\n";
                    break;
                }
                seg_map = read_segment(bvec, block_sz, false);
            }
            //bzero(buffer, segment_size);
            //bvec.clear();
            //buffer = bvec.data();
            if(seg_map["checksum_valid"] == "false")
                cout << "invalid checksum\n";
            else if(is_set_mss) {
                vector<unsigned char> ack = get_ack();
                //remove_nulls(ack);
                unsigned char* ack_bytes = ack.data();
                cout << "about to send ack\n";
                sendto(server_fd, ack_bytes, 8, 0, (const struct sockaddr *) &cli_addr, len);
                //send(new_socket, ack, 8, 0);
                is_set_mss = false;
            }
            else {
                if(seg_map["in_order"] == "true") {
                    string data = seg_map["data"];
                    std::ofstream out;
                    out.open(file_name, std::ios_base::app);
                    out << data;
                    out.close();
                }
                vector<unsigned char> ack = get_ack();
                unsigned char* ack_bytes = ack.data();
                sendto(server_fd, ack_bytes, 8, 0, (const struct sockaddr *) &cli_addr, len);
                //send(new_socket, ack, 8, 0);
            }
            if(is_resized) {
                bvec.resize(segment_size);
                is_resized = false;
            }
            bvec.clear();
            buffer = bvec.data();
            cout << "\n";
        }
        cout << "out of inner while\n";
    }
}
