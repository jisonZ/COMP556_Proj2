#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

unsigned short checksum(char *frame, int count) {
    unsigned long sum = 0;
    while (count--) {
        sum += *frame++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++; 
        }
    }
    return (sum & 0xFFFF);
}

void encode_ACK() {
    /* 
    int seqNum;
    int error;
    int eof;
    int checkSum;
    */
}

int decode_ACK(char* buf, int len) {
    /* TODO: is detecting length correct?*/
    /* Reasoning: UDP itself is used for tranfering long msgs, 
    therefore we dont care */
    /* return -1: not enough byte*/
    if (len != 12) return -1;
    int seqNum = ntohl(*(int*)buf);
    int error = ntohl(*((int*)buf+1));
    int eof = ntohl(*((int*)buf+2));
    int checkSum =  ;
    if ()
}

void encode_send() {
    /*
    int size;
    int seqNum;
    char* msg;
    int EOF;
    int checkSum;
    */
}

void decode_send() {

}

