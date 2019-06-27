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

umap Receiver::read_segment(unsigned char* segment, ssize_t num_bytes, bool is_set_mss) {
    umap map;
    if(is_set_mss) {
        if(segment[8] == '0')
            segment[8] = '\0';
        if(segment[9] == '0')
            segment[9] = '\0';
    }
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
        if(c == '0')
            c = '\0';
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
    else {
        map["in_order"] = "false";
        return map;
    }

    //vector<unsigned char> data_bytes;
    string data_str;
    for(int i = 8; i < num_bytes; i++) {
        char c = (char)segment[i];
        data_str += c;
    }
    map["data"] = data_str;
    
    if(is_set_mss)
        mss = stoi(data_str);
    return map;
}

bool Receiver::validate_checksum(unsigned char* segment, ssize_t num_bytes) {
    uint16_t checksum;
    if(segment[4] == '0')
        segment[4] = '\0';
    if(segment[5] == '0')
        segment[5] = '\0';
    checksum = segment[4];
    checksum = checksum << 8;
    checksum = checksum | segment[5];
    uint16_t concat;
    uint16_t sum;
    for(int i = 0; i < 4; i += 2) {
        if(segment[i] == '0')
            segment[i] = '\0';
        concat = segment[i];
        concat = concat << 8;
        concat = concat | segment[i + 1];
        sum += concat;
    }
    concat = segment[6];
    concat = concat << 8;
    concat = concat | segment[7];
    sum += concat;
    for(int i = 8; i < num_bytes; i++) {
        concat = segment[i];
        if( ++i == num_bytes)
            concat = concat << 8;
        else
            concat = (concat << 8) | segment[i];
        sum += concat;
    }
    uint16_t val = sum + checksum;
    return val == VALID_CHECKSUM;
}

unsigned char* Receiver::get_ack() {    
    // convert seq num to bool
    bool snb[32] = {false};
    int  i = 0;
    while(next_seq_num) {
        if (next_seq_num&1)
            snb[i] = 1;
        else
            snb[i] = 0;
        next_seq_num>>=1;
        i++;
    }
    std::reverse(std::begin(snb),std::end(snb));
    unsigned char* ack_bytes = new unsigned char[8];
    bool bool_byte[8] = {false};
    unsigned char seg_byte;
    int byte_num = 0;
    for(int i = 0; i < 32; i += 8) {
        bool_byte[i] = snb[i];
        if(i % 8 == 1) {      
             seg_byte = to_byte(bool_byte);
             byte_num = (i + 1)/8;
        }
        ack_bytes[byte_num - 1] = seg_byte;
    }
    ack_bytes[4] = ACK_Segment::zeroes[0];
    ack_bytes[5] = ACK_Segment::zeroes[1];
    ack_bytes[6] = ACK_Segment::type[0];
    ack_bytes[7] = ACK_Segment::type[1];
    return ack_bytes;
    //string ack_str = "Seq num: " + seq_bytes + "\r\n" + ACK_Segment::zeroes + "\r\n" + ACK_Segment::type + "\r\n"; 
}

void Receiver::download_file() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int segment_size = 20;
    umap seg_map;
    unsigned char buffer[20] = {0};
    ACK_Segment::init_static();
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);
    if((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // bind socket to port
    bzero(buffer, segment_size);
    ssize_t block_sz = 0;
    bool is_set_mss = true;
    if(bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    /* 
    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                       (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 
    */
    while(true) {
        while((block_sz = read(server_fd, buffer, segment_size)) > 0) {
            cout << "Received data - bytes " << std::to_string(block_sz) << "\n";
            double rand_val = dis(gen);
            if(rand_val <= loss_prob)
                continue;
            //string segment(buffer);
            if(is_set_mss) {
                seg_map = read_segment(buffer, block_sz, true);
                cout << "incoming segment set mss true - seq num " << seg_map["seq_num"] << std::endl;
                segment_size = mss + HEADER_SIZE;
                buffer[segment_size] = {0};
                //is_set_mss = false;
            }
            else {
                if(block_sz == 0) {
                    cout << "received no data\n";
                    break;
                }
                seg_map = read_segment(buffer, block_sz, false);
                cout << "incoming segment set mss false - seq num " << seg_map["seq_num"] << std::endl;
            }
            bzero(buffer, segment_size);
            if(seg_map["checksum_valid"] == "false")
                cout << "invalid checksum\n";
            else if(is_set_mss) {
                unsigned char* ack = get_ack();
                send(new_socket, ack, 8, 0);
                delete[] ack;
                is_set_mss = false;
            }
            else {
                if(seg_map["in_order"] == "true") {
                    string data = seg_map["data"];
                    std::ofstream out;
                    out.open(file_name, std::ios_base::app);
                    out << data;
                    out.close();
                    unsigned char* ack = get_ack();
                    send(new_socket, ack, 8, 0);                    
                    delete[] ack;
                }
                else {
                    unsigned char* ack = get_ack();
                    send(new_socket, ack, 8, 0);
                    delete[] ack;
                }
            }
        }
    }
}
