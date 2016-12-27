#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>

#include "packet.h"

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr,\
            "usage: ./sender [my port] [agent port] [dest port] [file]\n");
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
    socklen_t addr_len = sizeof(agent_addr);
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

    // get file size
    int fd = fileno(fp);
    struct stat buf;
    fstat(fd, &buf);
    int file_size = buf.st_size;

    // read all file into an array
    char* file_arr = (char*)malloc(file_size + 100);
    if (file_arr == NULL) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    nBytes = fread(file_arr, 1, file_size, fp);
    if (nBytes != file_size) {
        fprintf(stderr, "cannot read in all file into file_arr\n");
        exit(1);
    }

    // sending packet to agent
    int cnt = 0;
    packet send_pkt, recv_pkt;
    memset(&send_pkt, 0, sizeof(packet));
    memset(&recv_pkt, 0, sizeof(packet));
    send_pkt.from_port_no = my_port_no;
    send_pkt.to_port_no = dest_port_no;

    // transfer configuration
    int WAIT_ACK = 0;
    int WAIT_FIN = 0;
    int ALL_SENT = 0;
    int SEQ_NO = 0;
    int FILE_OFFSET = 0;
    int RETRANS = 0;

    // set timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,\
        (char*)&tv, sizeof(struct timeval));   

	while(1) {
        if (WAIT_FIN) {
            recvfrom(sockfd, &recv_pkt, sizeof(packet), 0,\
                (struct sockaddr*)&agent_addr, &addr_len);
            if (recv_pkt.is_ACK == 1 && recv_pkt.is_FIN == 1) {
                printf("[sender] recv\tfinack\n");
                break;
            }
        }
        else if (WAIT_ACK) {
            // wait for ACK
            recvfrom(sockfd, &recv_pkt, sizeof(packet), 0,\
                (struct sockaddr*)&agent_addr, &addr_len);

            // check timeout
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;
                printf("[sender] time\tout\n");
                WAIT_ACK = 0;
                RETRANS = 1;
                continue;
            }

            // receive packet
            if (recv_pkt.is_ACK == 1) {
                printf("[sender] recv\tACK\t#%d\n", recv_pkt.seq_no);
                WAIT_ACK = 0;
            }
        }
        else if (ALL_SENT && RETRANS == 0) {
            send_pkt.is_FIN = 1;
            ALL_SENT = 0;
            WAIT_FIN = 1;
		    sendto(sockfd, &send_pkt, sizeof(send_pkt), 0,\
                (struct sockaddr*)&agent_addr, sizeof(agent_addr));
            printf("[sender] send\tfin\n");
        }
        else {
            if (RETRANS) {
                sendto(sockfd, &send_pkt, sizeof(send_pkt), 0,\
                    (struct sockaddr*)&agent_addr, sizeof(agent_addr));
		        printf("[sender] resnd\tdata\t#%d\n", send_pkt.seq_no);
                WAIT_ACK = 1;
                RETRANS = 0;
                continue;
            }

            SEQ_NO = ++cnt;
            
            send_pkt.seq_no = SEQ_NO;
            int last_size = file_size - FILE_OFFSET;
            if (last_size > SEG_SIZE) {
                memcpy(send_pkt.content, file_arr+FILE_OFFSET, SEG_SIZE);
                send_pkt.len = SEG_SIZE;
                FILE_OFFSET += SEG_SIZE;
            }
            else {
                memcpy(send_pkt.content, file_arr+FILE_OFFSET, last_size);
                send_pkt.len = last_size;
                ALL_SENT = 1;
            }
		    
            sendto(sockfd, &send_pkt, sizeof(send_pkt), 0,\
                (struct sockaddr*)&agent_addr, sizeof(agent_addr));
		    printf("[sender] send\tdata\t#%d\n", send_pkt.seq_no);
            WAIT_ACK = 1;
        }
	}

    fclose(fp);

    return 0;
}

