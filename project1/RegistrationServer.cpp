#include "RegistrationServer.h"

RegistrationServer::RegistrationServer(): peer_list(std::vector<PeerDetails>()) {
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    std::string s(hostname);
    host_name = s;
}

std::string const RegistrationServer::response_str = "P2P-DI/1.0 <status_code> <phrase> \r\nContent-Length: <LENGTH>\r\nCOOKIE: <COOKIE>\r\nDATE: <DATE>\r\nPORT: <PORT>\r\nHOST: <HOST>\r\n";

void RegistrationServer::run_thread() {
    th = std::thread(&RegistrationServer::start_server, this);
    //th.join();
}

void RegistrationServer::start_server() {
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
    
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // set socket to accept multiple connections
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("rs setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // bind socket to port
    if(bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // 3 is how many pending connections queue will hold
    if(listen(server_fd, 300) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
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
            //printf("New connection , socket fd is %d , ip is : %s , port : %d  \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
            //       (address.sin_port));
            
            int length = 1024;
            bzero(buffer, length);
            int block_sz = 0;
            std::string req = "";
            while((block_sz = read(new_socket, buffer, 1024)) > 0) {
                std::string chunk(buffer);
                req += chunk;
                bzero(buffer, length);
                if(block_sz == 0 || block_sz != length)
                    break;
            }
            std::cout << "Registration server incoming request\n" << req << "\n";
            std::unordered_map<std::string, std::string> req_map = read_request(req);
            std::string response_str;
            if(req_map["type"] == "REGISTER")
                response_str = register_peer(req_map);
            else if(req_map["type"] == "STOP") {
                response_str = get_stop_response(req_map);
                const char* c_res_str = response_str.c_str();
                send(new_socket, c_res_str, strlen(c_res_str), 0);
                break;
            }
            else if(req_map["type"] == "PQUERY")
                response_str = pquery(req_map);
            else if(req_map["type"] == "LEAVE")
                response_str = leave(req_map);
            else if(req_map["type"] == "KEEPALIVE")
                response_str = keep_alive(req_map);
            const char* c_res_str = response_str.c_str();
            send(new_socket, c_res_str, strlen(c_res_str), 0);
        }
    }
    close(server_fd);
    return;
}

std::string PeerDetails::to_string() const {
    std::string str = "host:" + host_name + " port:" + std::to_string(port) + " peer_name:" + peer_name;
    return str;
}

std::string RegistrationServer::leave(std::unordered_map<std::string, std::string> &request) {
    std::string name = request["PEER_NAME"];
    trim(name);
    auto it = find_if(peer_list.begin(), peer_list.end(), [&name](const PeerDetails& p){return p.peer_name == name;});
    (*it).is_active = false;
    std::string res = get_response_string(200, "LEFT", *it);
    return res;
}

std::string RegistrationServer::keep_alive(std::unordered_map<std::string, std::string> &request) {
    std::string name = request["PEER_NAME"];
    trim(name);
    auto it = find_if(peer_list.begin(), peer_list.end(), [&name](const PeerDetails& p){return p.peer_name == name;});
    (*it).is_active = true;
    (*it).registration_time = time(0);
    std::string res = get_response_string(200, "ALIVE", *it);
    return res;
}

std::string RegistrationServer::pquery(std::unordered_map<std::string, std::string> &request) {
    std::string name = request["PEER_NAME"];
    trim(name);
    time_t now = time(0);
    auto it = find_if(peer_list.begin(), peer_list.end(), [&name](const PeerDetails& p){return p.peer_name == name;});
    std::string res = get_response_string(200, "OK", *it);
    std::string data;
    for(auto pd: peer_list) {
        if(pd.is_active) {
            if(now - pd.registration_time < 7200 && pd.peer_name != name)
                data += pd.to_string() + "\r\n";
            else
                pd.is_active = false;
        }
    }
    if(data.length() > 0)
        res += data + "\r\n";
    else {
        data = "No active peers\r\n";
        res += data;
    }
    replace(res, "Content-Length: 0", "Content-Length: " + std::to_string(data.size()));
    return res;
}

std::string RegistrationServer::register_peer(std::unordered_map<std::string, std::string> &request) {
    time_t now = time(0);
    char* dt = ctime(&now);
    std::string date_string(dt);
    trim(date_string);
    std::string thehost = request["HOST"];
    trim(thehost);
    std::string name = request["PEER_NAME"];
    trim(name);
    int port_num = stoi(request["PEER_SERVER_PORT"]);
    auto it = find_if(peer_list.begin(), peer_list.end(), [&name](const PeerDetails& p){return p.peer_name == name;});
    if(it != peer_list.end()) {
        PeerDetails pd = *it;
        pd.is_active = true;
        pd.ttl = 7200;
        pd.times_registered++;
        pd.datetime = date_string;
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
        pd.datetime = date_string;
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

std::string RegistrationServer::get_stop_response(std::unordered_map<std::string, std::string> &request) {
    std::string s = response_str;
    std::string name = request["PEER_NAME"];
    trim(name);
    auto it = find_if(peer_list.begin(), peer_list.end(), [&name](const PeerDetails& p){return p.peer_name == name;});
    PeerDetails pd = *it;
    return get_response_string(209, "STOPPED", pd);
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
    std::string::size_type pos = 0, prev = 0;
    std::string delimiter = "\r\n", type = "", method = "", val = "", key = "", data = "";
    int line_num = 0;
    while((pos = req.find(delimiter, prev)) != std::string::npos) {
        std::string line = req.substr(prev, pos-prev);
        //std::cout << "rs str to map " << line << "\n";
        if(line.length() ==  0)
            break;
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
