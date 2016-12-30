#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "packet.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage: ./agent [address] [port] [loss rate]\n");
        exit(1);
    }

    int sockfd;
    char* my_address = argv[1];
    int my_port_no = atoi(argv[2]);
    float loss_rate = atof(argv[3]);
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
	if (inet_pton(AF_INET, my_address, &agent.sin_addr) <= 0) {
		fprintf(stderr, "inet_pton() error\n");
		exit(1);
	}
    if (bind(sockfd, (struct sockaddr*)&agent, sizeof(agent)) == -1) {
        fprintf(stderr, "fail to bind port\n");
        exit(1);
    }

    // Initialize size variable to be used later on
    struct sockaddr_in sender;
    socklen_t sendsize = sizeof(sender);
    memset((char*)&sender, 0, sizeof(sender));

    // setup random drop
    srand(time(NULL));

    // receive a packet from sender
    packet pkt;
    float rnd;
    int pkt_cnt = 0;
    int drop_cnt = 0;

    while (1) {
        recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&sender, &sendsize);

        if (pkt.is_ACK == 0 && pkt.is_FIN == 0) {
            printf("[agent] get\tdata\t#%d\n", pkt.seq_no);
            pkt_cnt++;
            
            rnd = (float)(rand() % 100) / 100.0;
            if (rnd < loss_rate) {
                drop_cnt++;
                printf("[agent] drop\tdata\t#%d,\tloss rate = %.4f\n", pkt.seq_no, (float)drop_cnt/(float)pkt_cnt);
                continue;
            }
            else {
                printf("[agent] fwd\tdata\t#%d,\tloss rate = %.4f\n", pkt.seq_no, (float)drop_cnt/(float)pkt_cnt);
            }
        }
        else if (pkt.is_ACK == 1 && pkt.is_FIN == 0) {
            printf("[agent] get\tack\t#%d\n", pkt.seq_no);
            printf("[agent] fwd\tack\t#%d\n", pkt.seq_no);
        }
        else if (pkt.is_FIN == 1) {
            if (pkt.is_ACK == 0) {
                printf("[agent] get\tfin\n");
                printf("[agent] fwd\tfin\n");
            }
            else {
                printf("[agent] get\tfinack\n");
                printf("[agent] fwd\tfinack\n");
            }
        }

        sender.sin_family = AF_INET;
        sender.sin_port = htons(pkt.to_port_no);
        if (inet_pton(AF_INET, pkt.to_address, &sender.sin_addr) <= 0) {
            fprintf(stderr, "inet_pton() fail in while loop\n");
            exit(1);
        }

        sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&sender, sendsize);
    }

    return 0;
}

