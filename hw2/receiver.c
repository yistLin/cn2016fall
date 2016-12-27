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

    // transfer configuration
    int SEQ_NO = 1;

    while (1) {
        // receive packet
        recvfrom(sockfd, &recv_pkt, sizeof(packet), 0,\
            (struct sockaddr*)&agent, &addr_size);
        
        // receive out of order packet
        if (recv_pkt.seq_no != SEQ_NO && recv_pkt.is_FIN == 0) {
            printf("[receiver] send\tack\t#%d\n", ack_pkt.seq_no);
            sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0,\
                (struct sockaddr*)&agent, addr_size);
            continue;
        }

        // increase sequence number
        SEQ_NO++;

        // write data
        if (recv_pkt.is_FIN)
            printf("[receiver] recv\tfin\n");
        else {
            printf("[receiver] recv\tdata\t#%d\n", recv_pkt.seq_no);
            fwrite(recv_pkt.content, recv_pkt.len, 1, fp);
        }

        // send ACK back
        ack_pkt.seq_no = recv_pkt.seq_no;
        ack_pkt.from_port_no = port_no;
        ack_pkt.to_port_no = recv_pkt.from_port_no;
        ack_pkt.is_ACK = 1;
        ack_pkt.is_FIN = recv_pkt.is_FIN;
        sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0,\
            (struct sockaddr*)&agent, addr_size);
        
        // logging
        if (recv_pkt.is_FIN) {
            printf("[receiver] send\tfinack\n");
            break;
        }
        else
            printf("[receiver] send\tack\t#%d\n", ack_pkt.seq_no);
    }

    fclose(fp);

    return 0;
}

