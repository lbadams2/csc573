#include "Peer.h"

Peer::RFC_Client::RFC_Client(Peer &peer): parent(peer) {}

Peer::RFC_Server::RFC_Server(Peer &peer): parent(peer) {}

Peer::Peer(std::string the_host): rfc_client(*this), rfc_server(*this), host_name(the_host), server_port(-1), cookie(-1) {}

Peer::RFC_Client& Peer::get_rfc_client() {return rfc_client;}

std::string const Peer::RFC_Client::request_str = "<method> <document> P2P-DI/1.0 \r\nHOST: <host>\r\nSERVER_PORT: <server_port>\r\nCOOKIE: <cookie>\r\n";

void Peer::RFC_Client::request(std::string method) {
    std::string req_str = get_request_string(method);
    char const *req = req_str.c_str();
    send_request(req);
    //std::thread t(send_request, req, peer);
}

void Peer::RFC_Client::send_request(char const *req) {
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
    std::cout << req << std::endl;
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
    //valread = read(sock, buffer, 1024);
    close(sock);
}

std::string Peer::RFC_Client::get_request_string(std::string method) {
    std::string s = Peer::RFC_Client::request_str;
    replace(s, "<method>", "POST");
    replace(s, "<document>", method);
    replace(s, "<host>", parent.host_name);
    replace(s, "<server_port>", std::to_string(parent.server_port));
    replace(s, "<cookie>", std::to_string(parent.cookie));
    s += "\r\n";
    return s;
}

bool Peer::replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void Peer::RFC_Client::register_self() {
    
    //s.replace(s.find("<os>"), sizeof("<os>") - 1, parent.host_name);
}
