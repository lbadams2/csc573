#include "Peer.h"
#include "RegistrationServer.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
//using namespace std;

void test_task() {
    Peer::move_rfc_files("test");
    Peer *a = new Peer("peer_a");
    Peer::RFC_Client &a_client = a->get_rfc_client();
    Peer::RFC_Server &a_server = a->get_rfc_server();
    Peer *b = new Peer("peer_b");
    Peer::RFC_Client &b_client = b->get_rfc_client();
    Peer::RFC_Server &b_server = b->get_rfc_server();
    std::string str("none");
    std::thread th_a(&Peer::RFC_Client::request, a_client, "Register", str);
    std::thread th_b(&Peer::RFC_Client::request, b_client, "Register", str);


    std::thread stop_th(&Peer::RFC_Client::request, a_client, "Stop", str);
    th_a.join();
    th_b.join();

    delete a;
    delete b;
    
}

// /Users/liam_adams/my_repos/csc573/project1/rfc_files/rfc8598.txt
int main() {
    std::cout << "entered main\n";    
    RegistrationServer *rs = new RegistrationServer();
    std::thread rs_thread(&RegistrationServer::start_server, rs);
    std::cout << "rs thread running\n";
    test_task();

    rs_thread.join();
    delete rs;    
    return 0;
}
