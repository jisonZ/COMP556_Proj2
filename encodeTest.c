#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

int main() {
    // test send 

    int SEND_LEN = PKT_SIZE + FNAME_LEN + DIRNAME_LEN;
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

    // char *dirbuffer = (char *)malloc(DIRNAME_LEN);
    // if (!dirbuffer) {
    //     perror("failed to allocate msg buffer\n");
    //     abort();
    // }

    // char *fnamebuffer = (char *)malloc(FNAME_LEN);
    // if (!fnamebuffer) {
    //     perror("failed to allocate msg buffer\n");
    //     abort();
    // }

    char dirbuffer[DIRNAME_LEN];
    char fnamebuffer[FNAME_LEN];

    char testsend[] = "helloWorldnishishabi";
    char testfname[] = "fname";
    char testdir[] = "dirname";
    
    int seqnum = 5;
    encode_send(sendbuffer, seqnum, 1, testfname, testdir, testsend, 20);
    int getseq;
    int msgSize;
    int res = decode_send(sendbuffer, 20+16+FNAME_LEN+DIRNAME_LEN, &getseq, msgbuffer, &msgSize, dirbuffer, fnamebuffer);
    printf("%i\n", res);
    printf("%s\n", msgbuffer);
    printf("%i\n", msgSize);
    printf("%i\n", getseq);    
    printf("%s\n", fnamebuffer);
    printf("%s\n", dirbuffer);

}