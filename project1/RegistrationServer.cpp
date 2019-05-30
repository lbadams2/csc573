#include "RegistrationServer.h"

RegistrationServer::RegistrationServer() {
    start();
}

void RegistrationServer::start() {
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
    send(new_socket, hello, strlen(hello), 0);
    //printf("Hello message sent from server\n");
}