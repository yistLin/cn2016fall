#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage: ./sender [address] [port] [file]\n");
        exit(1);
    }

    char* server_addr = argv[1];
    int port_no = atoi(argv[2]);

    int sockfd, nBytes, ret;
	char buffer[1024];
	struct sockaddr_in recv_addr;
	socklen_t addr_size;

	// Create UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "fail to create socket, sockfd = %d\n", sockfd);
        exit(1);
    }

	// Configure settings in address struct
    memset((char*)&recv_addr, 0, sizeof(recv_addr));
	recv_addr.sin_family = AF_INET;
	recv_addr.sin_port = htons(port_no);
	// recv_addr.sin_addr.s_addr = htonl(server_addr);

	if ((ret = inet_pton(AF_INET, server_addr, &recv_addr.sin_addr)) <= 0) {
		fprintf(stderr, "inet_pton() error, ret = %d\n", ret);
		exit(1);
	}

	// Initialize size variable to be used later on
	addr_size = sizeof(recv_addr);

    int cnt = 0;
	while(1) {
		// printf("Type a sentence to send to server:\n");
		// fgets(buffer, 1024, stdin);

        sprintf(buffer, "the %d-th package\n", ++cnt);
		nBytes = strlen(buffer) + 1;

        sleep(1);

		// Send message to server
		sendto(sockfd, buffer, nBytes, 0,\
			(struct sockaddr*)&recv_addr, addr_size);

		// Receive message from server
		nBytes = recvfrom(sockfd, buffer, 1024, 0,\
			NULL, NULL);

		printf("Received from server: %s\n", buffer);
	}

    return 0;
}

