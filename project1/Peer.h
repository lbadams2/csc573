#ifndef Peer_h
#define Peer_h

#include <vector>
#include <string>

struct RFC_Record {
    int rfc_num;
    std::string title;
    std::string host_name;
    int ttl;
};

class Peer {
    public:
        Peer();

    private:
        std::vector<RFC_Record> rfc_index;
        class RFC_Server {
            public:
                RFC_Server();
                void start();
                void rfc_query();
                void get_rfc();
        };        
        class RFC_Client {
            public:
                RFC_Client();
                void start();
                void rfc_query();
                void get_rfc();
                void register_self();
                void leave();
                void pquery();
                void keep_alive();
        };
};

#endif /* RegistrationServer_h */