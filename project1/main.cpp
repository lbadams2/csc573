#include "Peer.h"
#include "RegistrationServer.h"
#include <iostream>
#include <thread>
//using namespace std;

void test_task() {
    // initialize directories and files for this task
    Peer::move_rfc_files("test");
    
    // construct peer objects
    Peer *a = new Peer("peer_a");
    Peer::RFC_Client &a_client = a->get_rfc_client();
    Peer::RFC_Server &a_server = a->get_rfc_server();
    Peer *b = new Peer("peer_b");
    Peer::RFC_Client &b_client = b->get_rfc_client();
    Peer::RFC_Server &b_server = b->get_rfc_server();
    
    //start peer servers
    std::thread th_a_server(&Peer::RFC_Server::start, a_server);
    std::thread th_b_server(&Peer::RFC_Server::start, b_server);
    
    // register peer servers
    std::unordered_map<std::string, std::string> arg_map;
    std::thread th_a_client(&Peer::RFC_Client::request, a_client, "Register", arg_map);
    std::thread th_b_client(&Peer::RFC_Client::request, b_client, "Register", arg_map);
    th_a_client.join();
    th_b_client.join();
    
    // query for peer list
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Pquery", arg_map);
    th_a_client.join();
    std::vector<Remote_Peer> peers = a->get_peer_index();
    //std::cout << "peer a index size " << peers.size() << "\n";
    
    // query peer for rfc list
    int remote_peer_port = peers[0].port;
    arg_map["PORT"] = std::to_string(remote_peer_port);
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Rfcquery", arg_map);
    th_a_client.join();
    std::vector<RFC_Record> rfc_list = a->get_rfc_index();
    //std::cout << "peer a rfc index size " << rfc_list.size() << "\n";
    
    // download rfc
    std::string title = rfc_list[0].title;
    arg_map["title"] = title;
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Getrfc", arg_map);
    th_a_client.join();
    
    // peer b leaves
    arg_map.clear();
    th_b_client = std::thread(&Peer::RFC_Client::request, b_client, "Leave", arg_map);
    th_b_client.join();
    
    // peer a query for peer list again
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Pquery", arg_map);
    th_a_client.join();
    
    // stop registration server
    arg_map.clear();
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Stop", arg_map);
    th_a_client.join();
    
    // stop peer A server
    arg_map["PORT"] = std::to_string(a->server_port);
    th_a_client = std::thread(&Peer::RFC_Client::request, a_client, "Stop_Peer", arg_map);
    th_a_client.join();
    th_a_server.join();
    
    // stop peer B server
    arg_map["PORT"] = std::to_string(b->server_port);;
    th_b_client = std::thread(&Peer::RFC_Client::request, b_client, "Stop_Peer", arg_map);
    th_b_client.join();
    th_b_server.join();
    delete a;
    delete b;
}

// /Users/liam_adams/my_repos/csc573/project1/rfc_files/rfc8598.txt
int main() {
    RegistrationServer *rs = new RegistrationServer();
    std::thread rs_thread(&RegistrationServer::start_server, rs);
    test_task();
    
    rs_thread.join();
    delete rs;
    return 0;
}
