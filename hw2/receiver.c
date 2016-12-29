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
        fprintf(stderr, "usage: ./my_addr [port] [file]\n");
        exit(1);
    }

    int port_no = atoi(argv[1]);
    char* filename = argv[2];
    int sockfd;
    struct sockaddr_in my_addr;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        error("fail to create socket");

    // configure settings in address struct
    memset((char*)&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_no);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket with address struct
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
        error("fail to bind port");

    // write to file
    FILE* fp;
    fp = fopen(filename, "wb");
    if (fp == NULL)
        error("fail to open file");

    // Initialize size variable to be used later on
    struct sockaddr_in agent;
    socklen_t addr_size = sizeof(agent);
    packet recv_pkt, ack_pkt;
    memset(&recv_pkt, 0, sizeof(packet));
    memset(&ack_pkt, 0, sizeof(packet));

    // Create data buffer
    char* data_buf = (char*)malloc(sizeof(char) * SEG_SIZE * BUF_SIZE);
    if (data_buf == NULL)
        error("fail to allocate memory");
    memset(data_buf, 0, sizeof(char) * SEG_SIZE * BUF_SIZE);

    // Create received data mapping
    char* rcvd_arr = (char*)malloc(sizeof(char) * BUF_SIZE);
    if (rcvd_arr == NULL)
        error("fail to allocate memory");
    memset(rcvd_arr, 0, sizeof(char) * BUF_SIZE);

    // transfer configuration
    int seq_no = 0;
    int seq_base = 0;
    int last_space = BUF_SIZE;
    int buf_len = 0;

    while (1) {
        // receive packet
        recvfrom(sockfd, &recv_pkt, sizeof(packet), 0, (struct sockaddr*)&agent, &addr_size);

        seq_no = recv_pkt.seq_no;

        if (recv_pkt.is_FIN == 0) {
            if (seq_no >= seq_base && seq_no < seq_base + BUF_SIZE) {
                // check if packet is received
                if (rcvd_arr[seq_no - seq_base] == 0) {
                    printf("[receiver] recv\tdata\t#%d\n", seq_no);
                    rcvd_arr[seq_no - seq_base] = 1;

                    last_space -= 1;
                    buf_len += recv_pkt.len;
                    int buf_offset = (seq_no - seq_base) * SEG_SIZE;
                    memcpy(data_buf+buf_offset, recv_pkt.content, recv_pkt.len);
                }
                else {
                    printf("[receiver] ignr\tdata\t#%d\n", seq_no);
                }
                
                // send ACK back
                ack_pkt.seq_no = seq_no;
                ack_pkt.from_port_no = port_no;
                ack_pkt.to_port_no = recv_pkt.from_port_no;
                ack_pkt.is_ACK = 1;
                ack_pkt.is_FIN = 0;
                sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&agent, addr_size);
                printf("[receiver] send\tack\t#%d\n", seq_no);
            }
            else {
                // out of buffer
                printf("[receiver] drop\tdata\t#%d\n", seq_no);

                // buffer full
                if (last_space == 0) {
                    printf("[receiver] flush\n");
                    fwrite(data_buf, 1, buf_len, fp);
                    buf_len = 0;
                    seq_base += BUF_SIZE;
                    last_space = BUF_SIZE;
                    memset(data_buf, 0, sizeof(char) * SEG_SIZE * BUF_SIZE);
                    memset(rcvd_arr, 0, sizeof(char) * BUF_SIZE);
                }
            }
        }
        else {
            // receive FIN
            printf("[receiver] recv\tfin\n");

            // flush the buffer
            printf("[receiver] flush\n");
            fwrite(data_buf, 1, buf_len, fp);

            // send FIN ACK
            printf("[receiver] send\tfinack\n");
            ack_pkt.is_ACK = 1;
            ack_pkt.is_FIN = 1;
            sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&agent, addr_size);

            // time to leave
            break;
        }
    }

    free(data_buf);
    free(rcvd_arr);

    fclose(fp);

    return 0;
}

