#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet.h"

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "usage: ./sender [my port] [agent port] [dest port] [file]\n");
        exit(1);
    }

    int my_port_no = atoi(argv[1]);
    int agent_port_no = atoi(argv[2]);
    int dest_port_no = atoi(argv[3]);
    char* filename = argv[4];

    int sockfd, nBytes, ret;
	struct sockaddr_in agent_addr, my_addr;

	// Create UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "fail to create socket, sockfd = %d\n", sockfd);
        exit(1);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port_no);

    // bind socket with address struct
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
        fprintf(stderr, "fail to bind port\n");
        exit(1);
    }

	// Configure settings to send to agent
    memset((char*)&agent_addr, 0, sizeof(agent_addr));
	agent_addr.sin_family = AF_INET;
	agent_addr.sin_port = htons(agent_port_no);
	if ((ret = inet_pton(AF_INET, "127.0.0.1", &agent_addr.sin_addr)) <= 0) {
		fprintf(stderr, "inet_pton() error, ret = %d\n", ret);
		exit(1);
	}

    // open file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "cannot open file\n");
        exit(1);
    }

    // sending packet to agent
    int cnt = 0;
    packet send_pkt, recv_pkt;
    memset(&send_pkt, 0, sizeof(packet));
    memset(&recv_pkt, 0, sizeof(packet));
    send_pkt.from_port_no = my_port_no;
    send_pkt.to_port_no = dest_port_no;

    int all_sent = 0;

	while(1) {
        if (all_sent == 1) {
            recvfrom(sockfd, &recv_pkt, sizeof(packet), 0, (struct sockaddr*)&agent_addr, sizeof(agent_addr));
            if (recv_pkt.is_ACK == 1) {
                printf("[sender] recv ACK, seq=%d\n", recv_pkt.seq_no);
                if (recv_pkt.is_FIN == 1)
                    break;
            }
        }
        else if (feof(fp)) {
            all_sent = 1;
            printf("[sender] file read completely.\n");
            send_pkt.is_FIN = 1;
		    sendto(sockfd, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr*)&agent_addr, sizeof(agent_addr));
        }
        else {
            send_pkt.seq_no = ++cnt;
            nBytes = fread(send_pkt.content, 1, sizeof(send_pkt.content), fp);
            send_pkt.len = nBytes;
		    sendto(sockfd, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr*)&agent_addr, sizeof(agent_addr));
		    printf("[sender] send %d\n", send_pkt.seq_no);
        }
	}

    fclose(fp);

    return 0;
}

