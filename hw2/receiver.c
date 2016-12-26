#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "packet.h"

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
    int sockfd;
    struct sockaddr_in receiver;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        error("fail to create socket");
    }

    // configure settings in address struct
    memset((char*)&receiver, 0, sizeof(receiver));
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port_no);
    receiver.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket with address struct
    if (bind(sockfd, (struct sockaddr*)&receiver, sizeof(receiver)) == -1) {
        error("fail to bind port");
        exit(1);
    }

    // Initialize size variable to be used later on
    // char buffer[1024];
    struct sockaddr_storage serverStorage;
    socklen_t addr_size = sizeof(serverStorage);

    packet pkt;

    while (1) {
        recvfrom(sockfd, &pkt, sizeof(packet), 0, (struct sockaddr*)&serverStorage, &addr_size);

        printf("[receiver] recv %d, %d\n", pkt.port_no, pkt.seq_no);

        // sendto(sockfd, buffer, nBytes, 0, (struct sockaddr*)&serverStorage, addr_size);
        break;
    }

    return 0;
}

