#include "Peer.h"
#include "RegistrationServer.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#define PORT 65423
//using namespace std;

// /Users/liam_adams/my_repos/csc573/project1/rfc_files/rfc8598.txt
int main() {
    std::cout << "entered main\n";
    
    RegistrationServer *rs = new RegistrationServer();
    std::thread rs_thread(&RegistrationServer::start_server, rs);
    std::cout << "rs thread running\n";
     
    //RegistrationServer rs;
    //rs.run_thread();
    
    /*
    Peer *a = new Peer("a.ncsu.edu");
    Peer::RFC_Client a_client = a->get_rfc_client();
    std::thread th(&Peer::RFC_Client::request, a_client, "Register");
    */
    
    
    Peer *stop = new Peer("stop.ncsu.edu");
    Peer::RFC_Client stop_client = stop->get_rfc_client();
    std::thread stop_th(&Peer::RFC_Client::request, stop_client, "Stop");
    std::cout << "joining stop thread\n";
    stop_th.join();
    std::cout << "stop thread joined\n";
    rs_thread.join();
    delete rs;
    delete stop;
    /*
    delete a;
    
    Peer b = Peer("b.ncsu.edu");
    Peer::RFC_Client b_client = b.get_rfc_client();
    b_client.request("Register");
     */
    return 0;
}
