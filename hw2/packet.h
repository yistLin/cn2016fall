#ifndef _PACKET_H
#define _PACKET_H

#define SEG_SIZE 1024
#define BUF_SIZE 32

typedef struct {
    unsigned int to_port_no;
    unsigned int from_port_no;

    unsigned int seq_no;
    unsigned int len;
    char content[SEG_SIZE];

    unsigned int is_ACK;
    unsigned int is_FIN;
} packet;

#endif

