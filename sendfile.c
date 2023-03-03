#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define MAX_BUFF 1000
#define PKT_SIZE 1024
#define WINDOW_LEN 5

struct packet 
{
    int seqNum;
    bool ack;
    bool send;
    timeval sendTime;
};

int main(int argc, char** argv) {

    char* serverAddr = NULL;
    unsigned short serverPort;
    char* fname = NULL;

    if (argc == 3) {
        serverAddr = argv[0];
        serverPort = atoi(argv[1]);
        fname = argv[2];
    } else {
        perror("usage: ./sendfile <server address> <server port> <window size> <file name>\n");
        abort();
    }

    /* ------ identifying the server ------ */
    unsigned int server_addr;
    struct sockaddr_in sin;
    struct addrinfo *getaddrinfo_result, hints;

    /* convert server domain name to IP address */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* indicates we want IPv4 */

    if (getaddrinfo(serverAddr, NULL, &hints, &getaddrinfo_result) == 0)
    {
        server_addr = (unsigned int)((struct sockaddr_in *)(getaddrinfo_result->ai_addr))->sin_addr.s_addr;
        freeaddrinfo(getaddrinfo_result);
    }
    
    /* ----- initialize and connect socket ------ */
    int sock;
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_TCP)) < 0)
    {
        perror("Error opening UDP socket");
        abort();
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = server_addr;
    sin.sin_port = htons(server_port);
    /* connect to the server */
    if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("connect to server failed");
        abort();
    }
    printf("Client is Connected\n");

    /* ----- create buffer ----- */
    char* buffer = (char *)malloc(MAX_BUFF*PKT_SIZE);
    if (!buffer) {
        perror("failed to allocated buffer");
        abort();
    }
    int bufferSize;

    /* ----- send file ----- */
    fd_set read_set;
    int max;
    timeval time_out;
    int select_retval;

    FILE* fp;
    fp = fopen(fname, "r");

    bool EOF = false;
    while (!EOF)
    {
        /* read from file to buffer */
        fgets(buffer, 255, (FILE*)fp);

        /* we only need to listen from the sockets */
        FD_ZERO(&read_set);
        FD_SET(sock, &read_set);
        max = sock;

        select_retval = select(max+1, &read_set, NULL, NULL, &timeout);

    }

}