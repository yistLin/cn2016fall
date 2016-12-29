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
#include <time.h>

#include "packet.h"

#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))

int main(int argc, char* argv[]) {
    if (argc < 5) {
        fprintf(stderr, "usage: ./sender [my port] [agent port] [more agent...(optional)] [dest port] [file]\n");
        exit(1);
    }

    int my_port_no = atoi(argv[1]);
    int total_agent = argc - 4;
    int* agent_port_arr = (int*)malloc(sizeof(int) * total_agent);
    if (agent_port_arr == NULL) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    for (int i = 0; i < total_agent; i++) {
        agent_port_arr[i] = atoi(argv[2+i]);
        printf("agent_port_arr[%d] = %d\n", i, agent_port_arr[i]);
    }
    srand(time(NULL));
    int dest_port_no = atoi(argv[argc-2]);
    char* filename = argv[argc-1];
    int sockfd, nBytes, ret;
	struct sockaddr_in agent_addr, my_addr;

	// Create UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "fail to create socket, sockfd = %d\n", sockfd);
        exit(1);
    }

    // Bind socket with address struct
    memset((char*)&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port_no);
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
        fprintf(stderr, "fail to bind port\n");
        exit(1);
    }

	// Configure settings to send to agent
    socklen_t addr_len = sizeof(agent_addr);
    memset((char*)&agent_addr, 0, sizeof(agent_addr));
	agent_addr.sin_family = AF_INET;
	agent_addr.sin_port = htons(agent_port_arr[0]);
	if ((ret = inet_pton(AF_INET, "127.0.0.1", &agent_addr.sin_addr)) <= 0) {
		fprintf(stderr, "inet_pton() error, ret = %d\n", ret);
		exit(1);
	}

    // Open file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "cannot open file\n");
        exit(1);
    }

    // Get file size
    int fd = fileno(fp);
    struct stat buf;
    fstat(fd, &buf);
    int file_size = buf.st_size;

    // Read all file into an array
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

    // Setup ACK array for data segments
    int seg_total = file_size / SEG_SIZE;
    if (file_size % SEG_SIZE != 0)
        seg_total += 1;
    char* ack_arr = (char*)malloc(sizeof(char) * seg_total);
    if (ack_arr == NULL) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(ack_arr, 0, sizeof(char) * seg_total);

    // Setup sent array
    char* sent_arr = (char*)malloc(sizeof(char) * seg_total);
    if (sent_arr == NULL) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(sent_arr, 0, sizeof(char) * seg_total);

    // Sending packet to agent
    packet send_pkt, recv_pkt;
    memset(&send_pkt, 0, sizeof(packet));
    memset(&recv_pkt, 0, sizeof(packet));
    send_pkt.from_port_no = my_port_no;
    send_pkt.to_port_no = dest_port_no;

    // Transfer state
    int WAIT_ACK = 0;
    int WAIT_FIN = 0;
    int RETRANS = 0;

    // Transfer config
    int window_size = 1;
    int threshold = 16;
    int seq_base = 0;
    int actual_sent = 0;

    // Set timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

	while(1) {
        if (WAIT_FIN) {
            // wait for FINACK
            recvfrom(sockfd, &recv_pkt, sizeof(packet), 0, (struct sockaddr*)&agent_addr, &addr_len);
            if (recv_pkt.is_ACK == 1 && recv_pkt.is_FIN == 1) {
                printf("[sender] recv\tfinack\n");
                break;
            }
        }
        else if (WAIT_ACK) {
            for (int i = 0; i < actual_sent; i++) {
                recvfrom(sockfd, &recv_pkt, sizeof(packet), 0, (struct sockaddr*)&agent_addr, &addr_len);

                // check timeout
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    errno = 0;
                    RETRANS = 1;
                    break;
                }

                // receive packet
                if (recv_pkt.is_ACK == 1) {
                    ack_arr[recv_pkt.seq_no] = 1;
                    printf("[sender] recv\tACK\t#%d\n", recv_pkt.seq_no);
                }
            }

            if (RETRANS) {
                int new_base = seq_base;
                int i;
                for (i = 0; i < actual_sent; i++) {
                    if (ack_arr[seq_base + i] == 0) {
                        new_base = seq_base + i;
                        break;
                    }
                }
                seq_base = new_base;
                threshold = MAX(window_size/2, 1);
                window_size = 1;
                printf("[sender] time\tout,\t\tthreshold = %d\n", threshold);
            }
            else {
                seq_base += actual_sent;
                window_size = (window_size >= threshold) ? window_size + 1 : window_size * 2;
            }

            RETRANS = 0;
            WAIT_ACK = 0;
        }
        else if (seq_base == seg_total) {
            // setup
            WAIT_FIN = 1;
            
            // send packet
            send_pkt.is_FIN = 1;
		    sendto(sockfd, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr*)&agent_addr, sizeof(agent_addr));
            printf("[sender] send\tfin\n");
        }
        else {
            // count actual sent packet
            actual_sent = 0;

            for (int i = 0; i < window_size; i++) {
                // sequence number
                send_pkt.seq_no = seq_base + i;

                // all packets sent
                if ((seq_base + i) == seg_total)
                    break;

                // get file content
                int file_offset = (seq_base + i) * SEG_SIZE;
                int send_size = file_size - file_offset;
                if (send_size >= SEG_SIZE)
                    send_size = SEG_SIZE;
                memcpy(send_pkt.content, file_arr+file_offset, send_size);
                send_pkt.len = send_size;

                // send packet
                agent_addr.sin_port = htons(agent_port_arr[rand() % total_agent]);
                sendto(sockfd, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr*)&agent_addr, sizeof(agent_addr));
                if (sent_arr[seq_base + i] == 0)
		            printf("[sender] send\tdata\t#%d,\twinSize = %d\n", send_pkt.seq_no, window_size);
                else
                    printf("[sender] resnd\tdata\t#%d,\twinSize = %d\n", send_pkt.seq_no, window_size);
                sent_arr[seq_base + i] = 1;
                actual_sent++;
            }

            // wait for #window_size ACKs
            WAIT_ACK = 1;
        }
	}

    // Release memory
    free(agent_port_arr);
    free(file_arr);
    free(ack_arr);
    free(sent_arr);

    fclose(fp);

    return 0;
}

