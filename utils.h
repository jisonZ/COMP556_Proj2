#ifndef _UTIL_C
#define _UTIL_C

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>

#define MAX_BUFF 1000
#define PKT_SIZE 1024
#define WINDOW_LEN 5
#define TIME_OUT 1
#define FNAME_LEN 20
#define DIRNAME_LEN 60

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec)
    {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000)
    {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

unsigned short checksum(char *frame, int count)
{
    unsigned long sum = 0;
    while (count--)
    {
        //printf("count\n");
        sum += *frame++;
        if (sum & 0xFFFF0000)
        {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return (sum & 0xFFFF);
}

unsigned short dummy_checksum(char *frame, int count){
    return 0;
}

void encode_ACK(int seqNum, int error, char *ack_buff)
{
    /*
    int seqNum;
    int error;
    int checkSum;
    */

    *((int *)ack_buff) = htonl(seqNum);
    *((int *)ack_buff + 1) = htonl(error);
    int checkSum = checksum(ack_buff, 8);
    *((int *)ack_buff + 2) = htonl(checkSum);
    return;
}

int decode_ACK(char *buf, int len, int* seqNum)
{
    /* Reasoning: UDP itself is used for tranfering long msgs,
    therefore we dont care */
    /* return -1: not enough byte recived or error*/
    if (len != 12)
        return -1;
    *seqNum = ntohl(*(int *)buf);
    int error = ntohl(*((int *)buf + 1));
    int checkSum = ntohl(*((int *)buf + 2));

    if (checksum(buf, 8) != checkSum)
    {
        return -1;
    }
    else if (error == 1)
    {
        return -2;
    }
    else
    {
        return 0;
    }
}

void encode_send(
    char *send_buff, 
    int seqNum, 
    int eof,
    char* fname,
    char* dir,
    char* msg,  
    int msgLen)
{
    /*
    int size;
    int seqNum;
    int eof;

    char* fname;
    char* dir;
    char* msg;

    int checkSum;
    */
    memset(send_buff, 0, PKT_SIZE+FNAME_LEN+DIRNAME_LEN+16);

    *((int *)send_buff) = htonl(16+msgLen+FNAME_LEN+DIRNAME_LEN);
    *((int *)send_buff + 1) = htonl(seqNum);
    *((int *)send_buff + 2) = htonl(eof);

    memcpy(send_buff + 12, fname, FNAME_LEN);
    memcpy(send_buff + 12 + FNAME_LEN, dir, DIRNAME_LEN);
    memcpy(send_buff + 12 + FNAME_LEN + DIRNAME_LEN, msg, msgLen);

    int checkSum = checksum(send_buff, 12+FNAME_LEN+DIRNAME_LEN+msgLen);
    *((int *)(send_buff + FNAME_LEN+DIRNAME_LEN+msgLen + 12)) = htonl(checkSum);
    return;
}

int decode_send(
    char *buf, 
    int len, 
    int *seqNum, 
    char *msg, 
    int *msgSize,
    char *dir,
    char *fname)
{
    int size = ntohl(*(int *)buf);

    // Check Size is  correct
    if (size != len)
        return -1;

    int checkSum = ntohl(*(int*)(buf + size - 4));
    // CheckSum Exam
    if (checkSum != checksum(buf, size - 4)) {
        // printf("actual checksum: %i, expected checksum: %i\n", checksum(buf, size - 4), checkSum);
        return -2;
    }

    // Copy fname and dir
    memcpy(fname, buf+12, FNAME_LEN);
    memcpy(dir, buf+12+FNAME_LEN, DIRNAME_LEN);

    // Copy MSG and its length
    memcpy(msg, buf+12+FNAME_LEN+DIRNAME_LEN, size-16-FNAME_LEN-DIRNAME_LEN);
    *msgSize = size-16-FNAME_LEN-DIRNAME_LEN;

    // set seqNum
    int seqnum = ntohl(*((int *)buf + 1));
    *seqNum = seqnum;

    // Return eof
    int eof = ntohl(*((int *)buf + 2));
    return eof;
}

#endif