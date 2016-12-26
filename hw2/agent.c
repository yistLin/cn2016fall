#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: ./agent [port]\n");
        exit(1);
    }

    int sockfd, ret;
    int my_port_no = atoi(argv[1]);
    struct sockaddr_in agent;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "fail to create socket\n");
        exit(1);
    }

    // configure settings in address struct
    memset((char*)&agent, 0, sizeof(agent));
    agent.sin_family = AF_INET;
    agent.sin_port = htons(my_port_no);

	if ((ret = inet_pton(AF_INET, "127.0.0.1", &agent.sin_addr)) <= 0) {
		fprintf(stderr, "inet_pton() error, ret = %d\n", ret);
		exit(1);
	}

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

    // receive a packet from sender
    packet pkt;

    while (1) {
        nBytes = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&sender, &sendsize);

        printf("%d %d\n", pkt.port_no, pkt.seq_no);

        // sendto(sockfd, buffer, nBytes, 0, (struct sockaddr*)&sender, sendsize);
    }

    return 0;
}

