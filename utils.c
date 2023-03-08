#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#define MAX_BUFF 1000
#define PKT_SIZE 1024
#define WINDOW_LEN 5
#define TIME_OUT 10

unsigned short checksum(char *frame, int count)
{
    unsigned long sum = 0;
    while (count--)
    {
        sum += *frame++;
        if (sum & 0xFFFF0000)
        {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return (sum & 0xFFFF);
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

void encode_send(int seqNum, char *send_buff, char *msg, int eof, int size)
{
    /*
    int size;
    int seqNum;
    int eof;
    char* msg;
    int checkSum;
    */

    *((int *)send_buff) = htonl(size + 16);
    *((int *)send_buff + 1) = htonl(seqNum);
    *((int *)send_buff + 2) = htonl(eof);
    *((char *)((int *)send_buff + 3)) = *msg;
    int checkSum = checksum(send_buff, 12 + size);
    *((int *)send_buff + size + 12) = htonl(checkSum);
    return;
}

int decode_send(char *buf, int len, int *seqNum, char *msg, int *msgSize)
{

    int size = ntohl(*(int *)buf);
    // Check Size is  correct
    if (size != len)
        return -1;

    int checkSum = ntohl(*(buf + size - 4));
    // CheckSum Exam
    if (checkSum != checksum(buf, size - 4))
        return -2;

    // Copy MSG and its length
    *msg = *(buf + 12);
    *msgSize = size - 16;

    // set seqNum
    int seqnum = ntohl(*((int *)buf + 1));
    *seqNum = seqnum;

    // Return eof
    int eof = ntohl(*((int *)buf + 2));
    return eof;
}
