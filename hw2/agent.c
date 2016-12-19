#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: ./agent [address] [port]\n");
        exit(1);
    }

    int sockfd;
    int port_no = atoi(argv[2]);
    struct sockaddr_in agent;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "fail to create socket\n");
        exit(1);
    }

    // configure settings in address struct
    memset((char*)&agent, 0, sizeof(agent));
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port_no);
    agent.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket with address struct
    if (bind(sockfd, (struct sockaddr*)&agent, sizeof(agent)) == -1) {
        fprintf(stderr, "fail to bind port\n");
        exit(1);
    }

    // Initialize size variable to be used later on
    int nBytes;
    char buffer[1024];
    struct sockaddr_in sender;
    socklen_t sendsize = sizeof(sender);
    memset((char*)&sender, 0, sizeof(sender));

    while (1) {
        nBytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sender, &sendsize);

        printf("%s", buffer);

        sendto(sockfd, buffer, nBytes, 0, (struct sockaddr*)&sender, sendsize);
    }

    return 0;
}

