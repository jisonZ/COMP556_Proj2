#include <stdlib.h>
#include <stdio.h>

char checksum(char *frame, int count) {
    u_long sum = 0;
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
    int size;
    int seqNum;
    int checkSum;
    */
}

void decode_ACK() {

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

