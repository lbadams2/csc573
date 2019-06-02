#include "RegistrationServer.h"

RegistrationServer::RegistrationServer(): peer_list(std::vector<PeerDetails>()) {}

std::string const RegistrationServer::response_str = "P2P-DI/1.0 <status_code> <phrase> \r\nContent-Length: <LENGTH>\r\nCOOKIE: <COOKIE>\r\nDATE: <DATE>\r\nPORT: <PORT>\r\nHOST: <HOST>\r\n";

void RegistrationServer::run_thread() {
    th = std::thread(&RegistrationServer::start_server, this);
    //th.join();
}

void RegistrationServer::start_server() {
    int server_fd, new_socket, valread, client_socket[30], max_clients = 30;
    int opt = 1, max_sd, i, sd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char const *hello = "Hello from server";
    // set of socket descriptors
    fd_set readfds;
    
    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }
    
    std::cout << "rs about to open socket\n";
    //std::cout.flush();
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "rs opened socket\n";
    //std::cout.flush();
    
    // set socket to accept multiple connections
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("rs setsockopt");
        exit(EXIT_FAILURE);
    }
    std::cout << "rs set sock opt\n";
    //std::cout.flush();
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // bind socket to port
    if(bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "rs performed bind\n";
    //std::cout.flush();
    
    // 3 is how many pending connections queue will hold
    if(listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "rs performed listen\n";
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
            if(req_map["type"] == "REGISTER")
                response_str = register_peer(req_map);
            else if(req_map["type"] == "STOP") {
                response_str = get_stop_response();
                const char* c_res_str = response_str.c_str();
                send(new_socket, c_res_str, strlen(c_res_str), 0);
                std::cout << "rs about to break\n";
                break;
            }
            else if(req_map["type"] == "PQUERY") {
                response_str = pquery(req_map);
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
    //printf("Hello message sent from server\n");
}

std::string PeerDetails::to_string() const {
    std::string str = "host:" + host_name + " port:" + std::to_string(port) + " peer_name:" + peer_name;
}

std::string RegistrationServer::leave(std::unordered_map<std::string, std::string> &request) {

}

std::string RegistrationServer::keep_alive(std::unordered_map<std::string, std::string> &request) {
    
}

std::string RegistrationServer::pquery(std::unordered_map<std::string, std::string> &request) {
    std::string thehost = request["HOST"];
    time_t now = time(0);
    auto it = find_if(peer_list.begin(), peer_list.end(), [&thehost](const PeerDetails& p){return p.host_name == thehost;});
    std::string res = get_response_string(200, "OK", *it);
    for(auto pd: peer_list) {
        if(pd.is_active) 
            if(now - pd.registration_time < 7200)
                res += pd.to_string() + "\r\n";
            else
                pd.is_active = false;
    }
    return res;
}

std::string RegistrationServer::register_peer(std::unordered_map<std::string, std::string> &request) {
    time_t now = time(0);
    char* dt = ctime(&now);
    std::string thehost = request["HOST"];
    std::string name = request["PEER_NAME"];
    int port_num = stoi(request["SERVER_PORT"]);
    auto it = find_if(peer_list.begin(), peer_list.end(), [&thehost](const PeerDetails& p){return p.host_name == thehost;});
    if(it != peer_list.end()) {
        PeerDetails pd = *it;
        pd.is_active = true;
        pd.ttl = 7200;
        pd.times_registered++;
        pd.datetime = dt;
        pd.port = port_num;
        pd.registration_time = now;
        return get_response_string(200, "UPDATED", pd);
    } else {
        PeerDetails pd = PeerDetails();
        pd.host_name = thehost;
        pd.peer_name = name;
        pd.is_active = true;
        pd.ttl = 7200;
        pd.times_registered = 1;
        pd.datetime = dt;
        pd.cookie = peer_list.size();
        pd.port = port_num;
        pd.registration_time = now;
        peer_list.push_back(pd);
        return get_response_string(204, "CREATED", pd);
    }
}

bool RegistrationServer::replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string RegistrationServer::get_stop_response() {
    std::string s = response_str;
    replace(s, "<status_code>", "209");
    replace(s, "<phrase>", "STOPPED");
    s += "\r\n";
    return s;
}

std::string RegistrationServer::get_response_string(int code, std::string phrase, PeerDetails &pd) {
    std::string s = response_str;
    replace(s, "<status_code>", std::to_string(code));
    replace(s, "<phrase>", phrase);
    replace(s, "<HOST>", host_name);
    replace(s, "<PORT>", std::to_string(PORT));
    replace(s, "<COOKIE>", std::to_string(pd.cookie));
    replace(s, "<DATE>", pd.datetime);
    replace(s, "<LENGTH>", "0");
    s += "\r\n";
    return s;
}

std::unordered_map<std::string, std::string> RegistrationServer::read_request(std::string &req) {
    std::unordered_map<std::string, std::string> map;
    std::string::size_type pos = 0, prev = 0, data_start = 0;
    std::string delimiter = "\r\n", type = "", method = "", val = "", key = "", data = "";
    int line_num = 0;
    bool is_data;
    while((pos = req.find(delimiter, prev)) != std::string::npos) {
        std::string line = req.substr(prev, pos-prev);
        std::cout << "rs str to map " << line << "\n";
        if(line.length() ==  0) {
            //is_data = true;
            //data_start = pos + delimiter.size();
            //data = req.substr(data_start + 1);
            //map["data"] = data;
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
                map["type"] = "REGISTER";
            else if(type == "L")
                map["type"] = "LEAVE";
            else if(type == "P")
                map["type"] = "PQUERY";
            else if(type == "K")
                map["type"] = "KEEPALIVE";
            else if(type == "S")
                map["type"] = "STOP";
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
RegistrationServer::~RegistrationServer() {
    if(_th.joinable())
        _th.join();
}
 */
