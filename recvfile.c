#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    struct sockaddr_in sin, addr;
    unsigned short server_port;

    if (argc == 2) {
        server_port =  = atoi(argv[1]);
    } else {
        perror("usage: ./recvfile <server port>\n");
        abort();
    }

    int sock, max;
    int optval = 1;

    /* socket address variables for a connected client */
    socklen_t addr_len = sizeof(struct sockaddr_in);

    /* maximum number of pending connection requests */
    int BACKLOG = 5;

    /* a buffer to read data */
    char *buf;
    int BUF_LEN = MaxBufSiz;

    buf = (char *)malloc(BUF_LEN);

    /* create a server socket to listen for TCP connection requests */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("opening TCP socket");
        abort();
    }

    /* set option so we can reuse the port number quickly after a restart */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        perror("setting TCP socket option");
        abort();
    }

    /* fill in the address of the server socket */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(server_port);

    /* bind server socket to the address */
    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("binding socket to address");
        abort();
    }

    /* variables for select */
    fd_set read_set, write_set;
    struct timeval time_out;
    int select_retval;

    