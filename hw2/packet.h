#ifndef _PACKET_H
#define _PACKET_H

typedef struct {
    unsigned int to_port_no;
    unsigned int from_port_no;

    unsigned int seq_no;
    unsigned int len;
    char content[32];

    unsigned int is_ACK;
    unsigned int is_FIN;
} packet;

#endif

