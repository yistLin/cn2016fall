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
    if (argc != 3) {
        fprintf(stderr, "usage: ./receiver [port] [file]\n");
        exit(1);
    }

    int port_no = atoi(argv[1]);
    char* filename = argv[2];
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

    // write to file
    FILE* fp;
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        error("fail to open file");
        exit(1);
    }

    // Initialize size variable to be used later on
    struct sockaddr_in agent;
    socklen_t addr_size = sizeof(agent);

    packet recv_pkt, ack_pkt;
    memset(&recv_pkt, 0, sizeof(packet));
    memset(&ack_pkt, 0, sizeof(packet));

    while (1) {
        recvfrom(sockfd, &recv_pkt, sizeof(packet), 0, (struct sockaddr*)&agent, &addr_size);

        if (recv_pkt.is_FIN == 1) {
            printf("[receiver] recv FIN\n");
        }
        else {
            printf("[receiver] recv %d\n", recv_pkt.seq_no);
            fwrite(recv_pkt.content, 1, recv_pkt.len, fp);
        }
        
        ack_pkt.seq_no = recv_pkt.seq_no;
        ack_pkt.from_port_no = port_no;
        ack_pkt.to_port_no = recv_pkt.from_port_no;
        ack_pkt.is_ACK = 1;
        ack_pkt.is_FIN = recv_pkt.is_FIN;
        sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&agent, addr_size);

        if (recv_pkt.is_FIN == 1) break;
    }

    fclose(fp);

    return 0;
}

