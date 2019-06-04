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
//#include <sys/stat.h>
#include <cstdlib>
#include <dirent.h>
#include <unordered_map>
#include <fstream>
#include <sstream>
#define RS_PORT 65423

struct RFC_Record {
    RFC_Record(int num, std::string thetitle, std::string thehost_name, std::string thepeer_name, int theport, int thettl, time_t time);
    RFC_Record();
    int rfc_num;
    std::string title;
    std::string host_name;
    std::string peer_name;
    int port;
    int ttl;
    time_t refresh_time;
    std::string to_string() const;
    bool operator==(const RFC_Record &rhs) {
        return title == rhs.title && peer_name == rhs.peer_name;
    }
};

struct Remote_Peer {
    //Remote_Peer(std::string host_name, std::string peer_name, int port);
    //Remote_Peer();
    std::string host_name;
    std::string peer_name;
    int port;
};

class Peer {
public:
    Peer(std::string the_peer);
    std::string host_name;
    std::string peer_name;
    int cookie;
    int server_port;
    static bool replace(std::string& str, const std::string& from, const std::string& to);
    static void move_rfc_files(std::string task);
    static void clean_dirs(std::string peer_name);
    
    class RFC_Server {
    public:
        RFC_Server(Peer &peer);
        static std::string const response_str;
        void start();
        std::string rfc_query(std::unordered_map<std::string, std::string> &request);
        std::string get_rfc(std::unordered_map<std::string, std::string> &request);
        std::unordered_map<std::string, std::string> read_request(std::string &req);
    private:
        Peer &parent;
        std::string get_response_string(int code, std::string phrase);
        static int available_port;
        static int get_port();
    };
    
    class RFC_Client {
    public:
        RFC_Client(Peer &peer);
        static std::string const request_str;
        void request(std::string method, std::unordered_map<std::string, std::string> args);
        std::string get_request_string(std::string method, std::unordered_map<std::string, std::string> args);
        void send_request(std::string &req, std::string &method, std::unordered_map<std::string, std::string> args);
    private:
        Peer &parent;
        std::string get_response_data(std::string &res);
        void rfc_query(std::string &res);
        void save_rfc(std::string &res, std::string &file_name);
        void pquery(std::string &res);
        void set_cookie(std::string &res);
    };
    
    Peer::RFC_Client& get_rfc_client();
    Peer::RFC_Server& get_rfc_server();
    std::vector<Remote_Peer>& get_peer_index();
    std::vector<RFC_Record>& get_rfc_index();
    
    static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
    }
    static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }
    static inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }
    
private:
    std::vector<RFC_Record> rfc_index;
    std::vector<Remote_Peer> peer_index;
    Peer::RFC_Client rfc_client;
    Peer::RFC_Server rfc_server;
};

#endif /* RegistrationServer_h */
