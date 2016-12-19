#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(char* msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: ./receiver [port]\n");
        exit(1);
    }

    int port_no = atoi(argv[1]);
    struct sockaddr_in server_info;

    int sockfd;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM,\
        IPPROTO_UDP)) == -1) {
        error("fail to create socket");
    }

    // configure settings in address struct
    memset((char*)&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(port_no);
    server_info.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket with address struct
    if (bind(sockfd, (struct sockaddr*)&server_info,\
        sizeof(server_info)) == -1) {
        error("fail to bind port");
    }

    // Initialize size variable to be used later on
    int nBytes;
    char buffer[1024];
    struct sockaddr_storage serverStorage;
    socklen_t addr_size = sizeof(serverStorage);

    while (1) {
        // Try to receive any incoming UDP datagram.
        // Address and port of requesting client
        // will be stored on serverStorage variable
        nBytes = recvfrom(sockfd, buffer, 1024, 0,\
            (struct sockaddr*)&serverStorage, &addr_size);

        printf("%s", buffer);

        // Send message back to client,
        // using serverStorage as the address
        sendto(sockfd, buffer, nBytes, 0,\
            (struct sockaddr*)&serverStorage, addr_size);
    }

    return 0;
}

