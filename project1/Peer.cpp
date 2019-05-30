#include "Peer.h"

Peer::Peer() {
    this->rfc_client = RFC_Client();
    std::cout << "constructed peer client";
    std::cout.flush();
}

Peer::RFC_Client::RFC_Client() {
    RFC_Client::start();
    std::cout << "executed peer start function";
    std::cout.flush();
}

void Peer::RFC_Client::start() {
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

    send(sock, hello, strlen(hello), 0);
    //std::cout << "Hello message sent from client\n";
    valread = read(sock, buffer, 1024);
    printf("print client buffer %s\n", buffer);
    close(sock);
}