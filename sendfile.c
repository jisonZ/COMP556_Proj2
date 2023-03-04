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
#define TIME_OUT 10
struct packet 
{
    int seqNum;
    int buffPos;
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
        perror("usage: ./sendfile <server address> <server port> <file name>\n");
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

    /* ----- send file ----- */
    fd_set read_set;
    fd_set write_set;
    int max;

    timeval time_out;
    time_out.tv_usec = 1e5;
    time_out.tv_sec = 0;

    int select_retval;

    FILE* fp;
    fp = fopen(fname, "r");

    int seq_num = 0;

    bool EOF = false;
    while (!EOF)
    {
        /* read from file to buffer */
        int bufferSize = fread(buffer, 1, MAX_BUFF*PKT_SIZE, fp);
        if (bufferSize < MAX_BUFF*PKT_SIZE) {
            EOF = true;
        }
        /* implement bufferSize/PKT_SIZE, EOF signal*/
        bufferSize /= PKT_SIZE;

        /* initialize window */
        int buff_pos = 0;
        packet* window = (packet*) malloc(WINDOW_LEN * sizeof(packet));
        if (window == NULL) {
            perror("fail to allocate window buffer\n");
            abort();
        }
        for (int i = 0; i < WINDOW_LEN; ++i) {
            struct packet p;
            p.seqNum = seq_num;
            p.buffPos = buff_pos;
            p.ack = false;
            p.send = false;
            window[i] = p;
            seq_num++;
            buff_pos++;
        }
 
         /* send buffer */
        while (true) 
        {
            /* we only need to listen from the sockets */
            FD_ZERO(&read_set);
            FD_ZERO(&wirte_set);

            FD_SET(sock, &read_set);
            FD_SET(sock, &write_set);
            max = sock;

            /* select() for ACK */
            select_retval = select(max+1, &read_set, &write_set, NULL, &timeout);
            if (select_retval < 0) {
                perror("select failed");
                abort();
            }
            if (select_retval > 0) {
                if (FD_ISSET(sock, &read_set)) {
                    /* take ACK */
                }
                if (FD_ISSET(sock, &write_set)) {
                    /* continue unsuccessful write */
                    /* might be too complicated to implement,
                    alternative is to wait till all of the message is sent */
                }
            }

            /* detect window shift */
            int shift = 0;
            for (int i = 0; i < WINDOW_LEN; ++i) {
                if (window[i].ack) {
                    shift++;
                } else {
                    break;
                }
            }
            if (shift != 0) {
                /* move window */
                for (int i = 0; i < WINDOW_LEN-shift; ++i) {
                    window[i] = window[i+shift];
                }

                /* reset rest of window */
                for (int i = WINDOW_LEN-shift; i < WINDOW_LEN; ++i) {
                    struct packet p;
                    p.seqNum = seq_num;
                    p.buffPos = buff_pos;
                    p.ack = false;
                    p.send = false;
                    window[i] = p;
                    seq_num++;
                    buff_pos++;
                }
            }

            /* detect if all item been sent*/
            if (window[0].buffPos > bufferSize) {
                break;
            }

            /* send frames */
            for (int i = 0; i < WINDOW_LEN; ++i) {
                struct timeval current_time;
                gettimeofday(&current_time, NULL);

                if (!window[i].ack || current_time - window[i].sendTime > TIME_OUT) {
                    /* fully send message */
                    if (EOF && window[i].buffPos == bufferSize - 1) {
                        /* send EOF in packet */
                    }
                }
            }
        }
    }

}