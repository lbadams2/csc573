#include "Peer.h"

RFC_Record::RFC_Record(int num, std::string thetitle, std::string thehost_name, std::string thepeer_name, int theport, int thettl, time_t time): rfc_num(num), title(thetitle), host_name(thehost_name), peer_name(thepeer_name), port(theport), ttl(thettl), refresh_time(time){}
RFC_Record::RFC_Record(){}
std::string RFC_Record::to_string() const {
    std::string str = "host:" + host_name + " port:" + std::to_string(port) + " title:" + title + " peer_name:" + peer_name;
}
//Remote_Peer::Remote_Peer(std::string host, std::string name, int theport): host_name(host), peer_name(name), port(theport){}
//Remote_Peer::Remote_Peer(){}

void Peer::clean_dirs(std::string peer_name) {
    std::string cmd = "rm -rf " + peer_name;
    system(cmd.c_str());
    cmd = "mkdir " + peer_name;
    system(cmd.c_str());
}

void Peer::move_rfc_files(std::string task) {
    struct dirent *entry;
    DIR *dp;
    dp = opendir("rfc_files");
    std::string cmd;
    if(task == "test") {
        clean_dirs("peer_a");
        clean_dirs("peer_b");
        int i = 0;
        while((entry = readdir(dp))) {
            if(i == 2)
                break;
            std::string fn(entry->d_name);
            cmd = "cp rfc_files/" + fn + " " + "peer_b";
            system(cmd.c_str());
            i++;
        }
    }
}

bool Peer::replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

Peer::Peer(std::string the_peer): rfc_client(*this), rfc_server(*this), peer_name(the_peer), cookie(-1) {}

Peer::RFC_Client& Peer::get_rfc_client() {return rfc_client;}
Peer::RFC_Server& Peer::get_rfc_server() {return rfc_server;}


/*
*
*   RFC_Server
*
*/
int Peer::RFC_Server::available_port = 65400;
std::string const Peer::RFC_Server::response_str = "P2P-DI/1.0 <status_code> <phrase> \r\nHost: <HOST>\r\nContent-Length: <LENGTH>\r\n";

int Peer::RFC_Server::get_port() {
    if(available_port != 65423)
        return available_port++;
    else
        return ++available_port;
}

Peer::RFC_Server::RFC_Server(Peer &peer): parent(peer) {
    server_port = get_port();
    struct dirent *entry;
    DIR *dp;
    dp = opendir(peer.peer_name.c_str());
    while((entry = readdir(dp))) {
        std::string fn(entry->d_name);
        int num = stoi(fn.substr(3, 4));
        std::string title = fn.substr(0, 7);
        time_t now = time(0);
        RFC_Record r(num, title, peer.host_name, peer.peer_name, server_port, 7200, now);
        peer.rfc_index.push_back(r);
    }
}

void Peer::RFC_Server::start() {
    int server_fd, new_socket, client_socket[30], max_clients = 30;
    int opt = 1, max_sd, i, sd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    // set of socket descriptors
    fd_set readfds;

    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }
    
    std::cout << "ps about to open socket\n";
    //std::cout.flush();
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "ps opened socket\n";
    //std::cout.flush();
    
    // set socket to accept multiple connections
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("rs setsockopt");
        exit(EXIT_FAILURE);
    }
    std::cout << "ps set sock opt\n";
    //std::cout.flush();
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server_port);
    
    // bind socket to port
    while(bind(server_fd, (struct sockaddr *) &address, sizeof(address)) != 0) {
        server_port = get_port();
        address.sin_port = htons(server_port);
    }
    std::cout << "ps performed bind\n";
    //std::cout.flush();
    
    // 3 is how many pending connections queue will hold
    if(listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "ps performed listen\n";
    //std::cout.flush();
    /*
     
     std::cout << "performed accept\n";*/
    //std::cout.flush();
    while(true) {
        // clear the socket set
        FD_ZERO(&readfds);
        // add listen socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        // 30 simultaneous connections allowed
        for(i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            if(sd > 0)
                FD_SET(sd, &readfds);
            if(sd > max_sd)
                max_sd = sd;
        }
        
        select(max_sd + 1, &readfds, NULL, NULL, NULL);
        // if something happened on server_fd (socket), then its an incoming connection
        if(FD_ISSET(server_fd, &readfds)) {
            if((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t*) &addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("New connection , socket fd is %d , ip is : %s , port : %d  \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
                   (address.sin_port));
            
            int length = 1024;
            bzero(buffer, length);
            int block_sz = 0;
            std::string req = "";
            std::cout << "rs starting to read request\n";
            while((block_sz = read(new_socket, buffer, 1024)) > 0) {
                std::string chunk(buffer);
                std::cout << "rs req chunk: " << chunk << "\n";
                req += chunk;
                bzero(buffer, length);
                if(block_sz == 0 || block_sz != length)
                    break;
            }
            std::cout << "rs done reading request\n";
            std::cout << "rs calling str to map\n";
            std::unordered_map<std::string, std::string> req_map = read_request(req);
            std::cout << "rs done str to map\n";
            std::string response_str;
            if(req_map["type"] == "RFCQUERY")
                response_str = rfc_query(req_map);
            else if(req_map["type"] == "GETRFC") {
                response_str = get_rfc(req_map);
            }
            else if(req_map["type"] == "STOP") {
                response_str = get_stop_response();
                const char* c_res_str = response_str.c_str();
                send(new_socket, c_res_str, strlen(c_res_str), 0);
                std::cout << "rs about to break\n";
                break;
            }
            const char* c_res_str = response_str.c_str();
            send(new_socket, c_res_str, strlen(c_res_str), 0);
            // read incoming message
            //valread = read(new_socket, buffer, 1024);
            // register, leave, pquery, or keep alive
        }
    }
    close(server_fd);
    std::cout << "rs broke out of while loop\n";
    return;
}

std::string Peer::RFC_Server::rfc_query(std::unordered_map<std::string, std::string> &request) {
    std::string res = get_response_string(200, "OK");
    time_t now = time(0);
    for(auto r: parent.rfc_index) {
        if(now - r.refresh_time < 7200)
            res += r.to_string() + "\r\n";
    }
    return res;
}

std::string Peer::RFC_Server::get_rfc(std::unordered_map<std::string, std::string> &request) {
    std::string res = get_response_string(200, "OK");
    time_t now = time(0);
    std::string title = request["title"];
    std::string name = parent.peer_name;
    auto it = find_if(parent.rfc_index.begin(), parent.rfc_index.end(), [&title, &name](const RFC_Record& r){return r.title == title && r.peer_name == name;});
    RFC_Record r = *it;
    std::string fn = r.peer_name + "/" + r.title + ".txt";
    std::ifstream ifs(fn);
    std::stringstream sstr;
    sstr << ifs.rdbuf();
    std::string file_string = sstr.str();
    res += file_string;
    return res;
}

std::string Peer::RFC_Server::get_response_string(int code, std::string phrase) {
    std::string s = response_str;
    replace(s, "<status_code>", std::to_string(code));
    replace(s, "<phrase>", phrase);
    replace(s, "<HOST>", parent.host_name);
    replace(s, "<LENGTH>", "0");
    s += "\r\n";
    return s;
}

std::string Peer::RFC_Server::get_stop_response() {
    std::string s = response_str;
    Peer::replace(s, "<status_code>", "209");
    Peer::replace(s, "<phrase>", "STOPPED");
    s += "\r\n";
    return s;
}

std::unordered_map<std::string, std::string> Peer::RFC_Server::read_request(std::string &req) {
    std::unordered_map<std::string, std::string> map;
    std::string::size_type pos = 0, prev = 0, data_start = 0;
    std::string delimiter = "\r\n", type = "", method = "", val = "", key = "", data = "";
    int line_num = 0;
    bool is_data;
    while((pos = req.find(delimiter, prev)) != std::string::npos) {
        std::string line = req.substr(prev, pos-prev);
        std::cout << "rs str to map " << line << "\n";
        if(line.length() ==  0) {
            break;
        }
        if(line_num == 0) {
            method = line.substr(0, 1);
            if(method == "P") {
                map["method"] = "POST";
                type = line.substr(5, 1);
            }
            else {
                map["method"] = "GET";
                type = line.substr(4, 1);
            }
            if(type == "R")
                map["type"] = "RFCQUERY";
            else if(type == "G")
                map["type"] = "GETRFC";
            line_num++;
            prev = pos + delimiter.size();
            continue;
        }
        key = line.substr(0, line.find(":"));
        val = line.substr(line.find(":") + 1);
        map[key] = val;
        prev = pos + delimiter.size();
    }
    return map;
}


/*
*
*   RFC_Client
*
*/
std::string const Peer::RFC_Client::request_str = "<method> <document> P2P-DI/1.0 \r\nHOST: <host>\r\nSERVER_PORT: <server_port>\r\nCOOKIE: <cookie>\r\nPEER_NAME: <peer_name>\r\n";

Peer::RFC_Client::RFC_Client(Peer &peer): parent(peer) {}

void Peer::RFC_Client::request(std::string method, std::string file_name) {
    std::string req_str = get_request_string(method);
    send_request(req_str, method, file_name);
}

void Peer::RFC_Client::send_request(std::string &req_str, std::string &method, std::string file_name) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char const *hello = "Hello from client";
    char buffer[1024] = {0};
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        exit(EXIT_FAILURE);
    }
    
    // copy 0 into serv_addr members
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(RS_PORT);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        exit(EXIT_FAILURE);
    }
    
    if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\n Connection failed \n";
        exit(EXIT_FAILURE);
    }
    //std::cout << req << std::endl;
    const char* req = req_str.c_str();
    send(sock, req, strlen(req), 0);
    //std::cout << "Hello message sent from client\n";
    int length = 1024;
    bzero(buffer, length);
    int block_sz = 0;
    std::string res = "";
    std::cout << "peer starting to read response\n";
    while((block_sz = read(sock, buffer, 1024)) > 0) {
        std::string chunk(buffer);
        std::cout << "peer res chunk: " << chunk << "\n";
        res += chunk;
        bzero(buffer, length);
        if(block_sz == 0 || block_sz != length)
            break;
    }
    close(sock);
    std::string res_data;
    if(method == "Pquery") {
        res_data = get_response_data(res);
        pquery(res_data);
    }
    else if(method == "Rfcquery") {
        res_data = get_response_data(res);
        rfc_query(res_data);
    }
    else if(method == "Getrfc") {
        res_data = get_response_data(res);
        save_rfc(res_data, file_name);
    }
    //return std::vector<Remote_Peer>();
}

void Peer::RFC_Client::save_rfc(std::string &res, std::string &file_name) {
    std::string path("downloads/" + parent.peer_name + "/" + file_name);
    std::ofstream out(path);
    out << res;
    out.close();
}

// host port title peername
void Peer::RFC_Client::rfc_query(std::string &res) {
    std::string::size_type pos = 0, prev = 0, line_prev = 0, line_pos = 0, word_pos = 0;
    std::string line, val, delimiter = "\r\n";
    while((pos = res.find(delimiter, prev)) != std::string::npos) {
        line = res.substr(prev, pos-prev);
        std::cout << "outer loop " << line << "\n";
        if(line.length() == 0)
            break;
        int i = 0;
        RFC_Record rfc;
        while((line_pos = line.find(":", line_prev)) != std::string::npos) {
            word_pos = line.find(" ", line_pos);
            val = line.substr(line_pos + 1, word_pos - line_pos - 1);
            if(i == 0)
                rfc.host_name = val;
            else if(i == 1)
                rfc.port = stoi(val);
            else if(i ==2)
                rfc.title = val;
            else
                rfc.peer_name = val;
            i++;
            line_prev = word_pos;
        }
        std::string name = rfc.peer_name;
        std::string t = rfc.title;
        auto it = find_if(parent.rfc_index.begin(), parent.rfc_index.end(), [&name, &t](const RFC_Record &r){return r.peer_name == name && r.title == t;});
        if(it != parent.rfc_index.end()) {
            (*it).refresh_time = time(0);
        } else {
            rfc.refresh_time = time(0);
            rfc.rfc_num = stoi(rfc.title.substr(3, 4));
            rfc.ttl = 7200;
            parent.rfc_index.push_back(rfc);
        }
        line_prev = 0;
        prev = pos + delimiter.size();
    }
}

void Peer::RFC_Client::pquery(std::string &res) {
    std::string::size_type pos = 0, prev = 0, line_prev = 0, line_pos = 0, word_pos = 0;
    std::string line, val, delimiter = "\r\n";
    std::vector<Remote_Peer> peers;
    while((pos = res.find(delimiter, prev)) != std::string::npos) {
        line = res.substr(prev, pos-prev);
        std::cout << "outer loop " << line << "\n";
        if(line.length() == 0)
            break;
        int i = 0;
        Remote_Peer rp;
        while((line_pos = line.find(":", line_prev)) != std::string::npos) {
            word_pos = line.find(" ", line_pos);
            val = line.substr(line_pos + 1, word_pos - line_pos - 1);
            if(i == 0)
                rp.host_name = val;
            else if(i == 1)
                rp.peer_name = val;
            else
                rp.port = stoi(val);
            i++;
            line_prev = word_pos;
        }
        peers.push_back(rp);
        line_prev = 0;
        prev = pos + delimiter.size();
    }
    parent.peer_index = peers;
    //return vec;
}

std::string Peer::RFC_Client::get_response_data(std::string &res) {
    std::string::size_type pos = 0, prev = 0;
    std::string delimiter = "\r\n", line;
    while((pos = res.find(delimiter, prev)) != std::string::npos) {
        line = res.substr(prev, pos-prev);
        if(line.length() == 0){
            prev = pos + delimiter.size();
            break;
        }
    }
    return res.substr(prev);
}

std::string Peer::RFC_Client::get_request_string(std::string method) {
    std::string s = Peer::RFC_Client::request_str;
    replace(s, "<method>", "POST");
    replace(s, "<document>", method);
    replace(s, "<host>", parent.host_name);
    replace(s, "<peer_name>", parent.peer_name);
    //replace(s, "<server_port>", std::to_string(parent.server_port));
    replace(s, "<cookie>", std::to_string(parent.cookie));
    s += "\r\n";
    return s;
}
