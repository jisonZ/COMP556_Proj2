#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

int main() {
    // test send 
    int RECV_LEN = 12;
    char *recvbuffer = (char *)malloc(RECV_LEN);
    if (!recvbuffer)
    {
        perror("failed to allocate recv buffer\n");
        abort();
    }

    int SEND_LEN = PKT_SIZE + 16;
    char *sendbuffer = (char *)malloc(SEND_LEN);
    if (!sendbuffer)
    {
        perror("failed to allocate send buffer\n");
        abort();
    }

    char *msgbuffer = (char *)malloc(PKT_SIZE);
    if (!msgbuffer) {
        perror("failed to allocate msg buffer\n");
        abort();
    }

    char testsend[] = "helloWorldnishishabi";
    int seqnum = 5;
    encode_send(seqnum, sendbuffer, testsend, 1, 20);
    int getseq;
    int msgSize;
    int res = decode_send(sendbuffer, 20+16, &getseq, msgbuffer, &msgSize);
    printf("%i\n", res);
    printf("%s\n", msgbuffer);
    printf("%i\n", msgSize);
    printf("%i\n", getseq);    
}