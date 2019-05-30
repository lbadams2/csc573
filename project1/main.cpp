//#include "Peer.h"
//#include "RegistrationServer.h"
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
void start_server() {
    int server_fd, new_socket, valread;
    int opt = 1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char const *hello = "Hello from server";
    
    std::cout << "about to open socket\n";
    //std::cout.flush();
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "opened socket\n";
    //std::cout.flush();

    // set socket to accept multiple connections
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("rs setsockopt");
        exit(EXIT_FAILURE);
    }
    std::cout << "set sock opt\n";
    //std::cout.flush();

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // bind socket to port
    if(bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "performed bind\n";
    //std::cout.flush();

    // 3 is how many pending connections queue will hold
    if(listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "performed listen\n";
    //std::cout.flush();

    if((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t*) &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    std::cout << "performed accept\n";
    //std::cout.flush();

    // from unistd.h
    valread = read(new_socket, buffer, 1024);
    printf("print server buffer: %s\n", buffer);

    char const *file_name = "/Users/liam_adams/my_repos/csc573/project1/rfc_files/rfc8598.txt";
    FILE *fp = fopen(file_name, "r");
    int f_block_sz;
    int length = 1024;
    char sdbuf[length];
    std::cout << "about to send file";
    while((f_block_sz = fread(sdbuf, sizeof(char), length, fp)) > 0) {
        if(send(new_socket, sdbuf, f_block_sz, 0) < 0) {
            perror("file send failed");
            exit(EXIT_FAILURE);
        }
        bzero(sdbuf, length);
    }
    fclose(fp);
    std::cout << "file sent";
    //send(new_socket, hello, strlen(hello), 0);
}

void start_peer() {
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
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        exit(EXIT_FAILURE);
    }

    if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\n Connection failed \n";
        exit(EXIT_FAILURE);
    }

    send(sock, hello, strlen(hello), 0);
    //std::cout << "Hello message sent from client\n";
    char const *download_file = "/Users/liam_adams/my_repos/csc573/project1/downloads/test.txt";
    FILE *fp = fopen(download_file, "a");
    int length = 1024;
    char rbuf[length];
    bzero(rbuf, length);
    int f_block_sz = 0;
    while((f_block_sz = read(sock, rbuf, 1024)) > 0) {
        int write_sz = fwrite(rbuf, sizeof(char), f_block_sz, fp);
        bzero(rbuf, length);
        if(f_block_sz == 0 || f_block_sz != length)
            break;
    }
    fclose(fp);
    close(sock);
    //valread = read(sock, buffer, 1024);
    printf("print client buffer: %s\n", rbuf);
}


int main() {
    std::cout << "entered main\n";
    //cout.flush();
    //printf("entered main\n");
    std::thread t1(start_server);
    std::cout << "started registration server\n";
    //cout.flush();
    std::cout << "about to start peer\n";
    start_peer();    
    //Peer p = Peer();
    std::cout << "started peer\n";
    t1.join();
    return 0;
}