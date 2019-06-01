#ifndef Peer_h
#define Peer_h

#include <vector>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>
#define RS_PORT 65423

struct RFC_Record {
    int rfc_num;
    char* title;
    char* host_name;
    int ttl;
};

class Peer {
    public:
        Peer(std::string the_host);
        std::string host_name;
        int server_port;
        int cookie;
        static bool replace(std::string& str, const std::string& from, const std::string& to);
        class RFC_Server {
            public:
                RFC_Server(Peer &peer);
                std::string const response_str = "P2P-DI/1.0 <status_code> <phrase> \r\nContent-Length: <length>\r\nOS: <os>";
                void start();
                void rfc_query();
                void get_rfc();
            private:
                Peer &parent;
        };
        class RFC_Client {
            public:
                RFC_Client(Peer &peer);
                static std::string const request_str;
                void request(std::string method);
                std::string get_request_string(std::string method);
                void send_request(char const *req);
                void run_thread(char const *req);
                void rfc_query();
                void get_rfc();
                void register_self();
                void leave();
                void pquery();
                void keep_alive();
            private:
                Peer &parent;
        };
        Peer::RFC_Client& get_rfc_client();
        Peer::RFC_Server& get_rfc_server();
    
    private:
        std::vector<RFC_Record> rfc_index;
        Peer::RFC_Client rfc_client;
        Peer::RFC_Server rfc_server;
};

#endif /* RegistrationServer_h */
