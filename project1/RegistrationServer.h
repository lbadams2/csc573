#ifndef RegistrationServer_h
#define RegistrationServer_h

#include <vector>
//#include <string>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
//#include <stdio.h>
#include <iostream>
#include <string.h>
#define PORT 65423
/*
time_t now = time(0);
char* dt = ctime(&now);
*/

struct PeerDetails {
    char* host_name;
    int cookie;
    bool is_active;
    int ttl;
    int port;
    int times_registered;
    char* datetime;
};

class RegistrationServer {
    public:
        RegistrationServer();
        void start();
        void register_peer();
        void leave();
        void pquery();
        void keep_alive();

    
    private:
        std::vector<PeerDetails> peer_list;

};

#endif /* RegistrationServer_h */