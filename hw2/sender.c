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
    if (argc != 4) {
        fprintf(stderr, "usage: ./sender [my port] [agent port] [dest port]\n");
        exit(1);
    }

    int my_port_no = atoi(argv[1]);
    int agent_port_no = atoi(argv[2]);
    int dest_port_no = atoi(argv[3]);

    int sockfd, nBytes, ret;
	char buffer[1024];
	struct sockaddr_in agent_addr;
	// socklen_t addr_size;

	// Create UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "fail to create socket, sockfd = %d\n", sockfd);
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

    // sending packet to agent
    packet pkt;
    pkt.port_no = my_port_no;
    pkt.seq_no = 1;

	while(1) {

        // sprintf(buffer, "the %d-th package\n", ++cnt);
		// nBytes = strlen(buffer) + 1;

		sendto(sockfd, &pkt, sizeof(pkt)+1, 0, (struct sockaddr*)&agent_addr, sizeof(agent_addr));

		// nBytes = recvfrom(sockfd, buffer, 1024, 0, NULL, NULL);

		// printf("Received from server: %s\n", buffer);

        break;
	}

    return 0;
}

