#ifndef RegistrationServer_h
#define RegistrationServer_h

#include <vector>
#include <string>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
//#include <stdio.h>
#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <thread>
#define PORT 65423
/*
 time_t now = time(0);
 char* dt = ctime(&now);
 */

struct PeerDetails {
    std::string host_name;
    int cookie;
    bool is_active;
    int ttl;
    int port;
    int times_registered;
    std::string datetime;
    time_t registration_time;
    
    bool operator==(const PeerDetails& rhs) const {
        return host_name == rhs.host_name;
    }
};

class RegistrationServer {
    public:
        RegistrationServer();
        //~RegistrationServer();
        //RegistrationServer(const RegistrationServer &old_obj);
        void run_thread();
        void start_server();
        static std::string const response_str;
        std::string host_name;
        std::unordered_map<std::string, std::string> read_request(std::string &req);
        static bool replace(std::string& str, const std::string& from, const std::string& to);
    
    private:
        std::string register_peer(std::unordered_map<std::string, std::string> &request);
        void leave();
        void pquery();
        void keep_alive();
        std::string get_response_string(int code, std::string phrase, PeerDetails &pd);
        std::string get_stop_response();
        std::vector<PeerDetails> peer_list;
        std::thread th;
};

#endif /* RegistrationServer_h */
