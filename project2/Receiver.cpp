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

Receiver::Receiver(string fn, double lp): file_name(fn), loss_prob(lp) {}

umap Receiver::read_segment(unsigned char* segment, bool is_set_mss) {
    umap map;
    std::string::size_type pos = 0, prev = 0;
    string delimiter = "\r\n", line;    
    int line_num = 0;
    uint16_t checksum;
    while((pos = segment.find(delimiter, prev)) != std::string::npos) {
        line = segment.substr(prev, pos-prev);
        if(line.length() == 0){
            prev = pos + delimiter.size();
            break;
        }
        if(line_num == 0)
            map["seq_num"] = line.substr(line.find(":") + 1);
        else if(line_num == 1)
            map["source_port"] = line.substr(line.find(":") + 1);
        else if(line_num == 2)
            map["dest_port"] = line.substr(line.find(":") + 1);
        else if(line_num == 3) {
            string l = line.substr(line.find(":") + 1);
            map["length"] = l;
            if(is_set_mss) {
                std::stringstream lstream(l);
                lstream >> mss;
            }
        }
        else if(line_num == 4) {
            string cs = line.substr(line.find(":") + 1);
            std::stringstream csstream(cs);
            csstream >> checksum;
        }
        line_num++;
        prev = pos + delimiter.size();
    }
    map["data"] = segment.substr(prev);
    uint16_t seq_num = stoi(map["seq_num"]);
    if(seq_num == next_seq_num)
        map["in_order"] = "true";
    else
        map["in_order"] = "false";
    bool is_valid = validate_checksum(checksum, map);
    if(is_valid)
        map["checksum_valid"] = "true";
    else
        map["checksum_valid"] = "false";
    if(is_set_mss)
        set_mss(map["data"]);
    return map;
}

void Receiver::set_mss(string data)  {
    string ms = data.substr(data.find(":") + 1);
    std::stringstream msstream(ms);
    msstream >> mss;
}

bool Receiver::validate_checksum(uint16_t checksum, umap &seg_map) {
    uint16_t sn = stoi(seg_map["seq_num"]);
    uint16_t sp = stoi(seg_map["source_port"]);
    uint16_t dp = stoi(seg_map["dest_port"]);
    uint16_t l = stoi(seg_map["length"]);
    string s = seg_map["data"];
    std::vector<char> data(s.begin(), s.end());
    uint16_t data_sum;
    for(auto it = data.begin(); it < data.end(); it++) {
        data_sum = *it;
        it++;
        if(it == data.end())
            data_sum = data_sum << 8;
        else
            data_sum = (data_sum << 8) | *it;
        data_sum += data_sum;
    }
    uint16_t val = sn + sp + dp + l + data_sum + checksum;
    return val == VALID_CHECKSUM;
}

unsigned char* Receiver::get_ack() {    
    // convert seq num to bool
    bool snb[32];
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
    bool bool_byte[8];
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
    int block_sz = 0;
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
            cout << "Received data\n";
            double rand_val = dis(gen);
            if(rand_val <= loss_prob)
                continue;
            string segment(buffer);
            if(is_set_mss) {
                seg_map = read_segment(segment, true);
                cout << "incoming segment set mss true - seq num " << seg_map["seq_num"] << std::endl;
                segment_size = mss + HEADER_SIZE;
                buffer[segment_size] = {0};
                is_set_mss = false;
            }
            else {
                seg_map = read_segment(segment, false);
                cout << "incoming segment set mss false - seq num " << seg_map["seq_num"] << std::endl;
                if(block_sz == 0) {
                    cout << "received no data\n";
                    break;
                }
            }
            bzero(buffer, segment_size);
            if(seg_map["checksum_valid"] == "false")
                cout << "invalid checksum\n";
            else {
                if(seg_map["in_order"] == "true") {
                    string data = seg_map["data"];
                    std::ofstream out;
                    out.open(file_name, std::ios_base::app);
                    out << data;
                    out.close();
                    unsigned char* ack = get_ack();
                    send(new_socket, ack, 8, 0);
                    next_seq_num += mss;
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
