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
#include "utils.c"

struct packet
{
    int seqNum;
    int buffPos;
    bool ack;
    bool send;
    timeval sendTime;
};

int main(int argc, char **argv)
{

    char *serverAddr = NULL;
    unsigned short serverPort;
    char *fname = NULL;

    if (argc == 3)
    {
        serverAddr = argv[0];
        serverPort = atoi(argv[1]);
        fname = argv[2];
    }
    else
    {
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
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
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
    char *filebuffer = (char *)malloc(MAX_BUFF * PKT_SIZE);
    if (!buffer)
    {
        perror("failed to allocate file buffer\n");
        abort();
    }
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
    if (!msgbuffer)
    {
        perror("failed to allocate msg buffer\n");
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

    FILE *fp;
    fp = fopen(fname, "r");

    int seq_num = 0;

    bool EOF = false;

    while (!EOF)
    {
        /* read from file to buffer */
        int bufferSize = fread(filebuffer, 1, MAX_BUFF * PKT_SIZE, fp);
        if (bufferSize < MAX_BUFF * PKT_SIZE)
        {
            EOF = true;
        }
        /* implement bufferSize/PKT_SIZE, EOF signal*/
        int bufferCount = bufferSize /= PKT_SIZE;

        /* initialize window */
        int buff_pos = 0;
        packet *window = (packet *)malloc(WINDOW_LEN * sizeof(packet));
        if (window == NULL)
        {
            perror("fail to allocate window buffer\n");
            abort();
        }
        for (int i = 0; i < WINDOW_LEN; ++i)
        {
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
        while (1)
        {
            /* we only need to listen from the sockets */
            FD_ZERO(&read_set);
            // FD_ZERO(&wirte_set);

            FD_SET(sock, &read_set);
            // FD_SET(sock, &write_set);
            max = sock;

            /* select() for ACK */
            select_retval = select(max + 1, &read_set, NULL, NULL, &timeout);
            if (select_retval < 0)
            {
                perror("select failed");
                abort();
            }
            if (select_retval > 0)
            {
                if (FD_ISSET(sock, &read_set))
                {
                    /* take ACK */
                    int recvLen = recv(sock, recvbuffer, RECV_LEN, 0); // Ack
                    int ack_num;
                    int ack_status = decode_ACK(recvbuffer, recvLen, &ack_num);

                    int first_window_seq = window[0].seqNum;
                    int last_window_seq = window[WINDOW_LEN - 1].seqNum;
                    int window_pos = ack_num - first_window_seq;
                    if (ack_num >= first_window_seq || ack_num <= last_window_seq)
                    {
                        if (ack_status == -2)
                        {
                            window[window_pos].send = false;
                        }
                        else if (ack_status == 0)
                        {
                            window[ack_num - first_window_seq].ack = true;
                        }
                    }
                    // if (FD_ISSET(sock, &write_set)) {
                    //     /* continue unsuccessful write */
                    //     /* might be too complicated to implement,
                    //     alternative is to wait till all of the message is sent */
                    // }
                }

                /* detect window shift */
                int shift = 0;
                for (int i = 0; i < WINDOW_LEN; ++i)
                {
                    if (window[i].ack)
                    {
                        shift++;
                    }
                    else
                    {
                        break;
                    }
                }
                if (shift != 0)
                {
                    /* move window */
                    for (int i = 0; i < WINDOW_LEN - shift; ++i)
                    {
                        window[i] = window[i + shift];
                    }

                    /* reset rest of window */
                    for (int i = WINDOW_LEN - shift; i < WINDOW_LEN; ++i)
                    {
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
                if (window[0].buffPos > bufferCount)
                {
                    break;
                }

                /* send frames */
                for (int i = 0; i < WINDOW_LEN; ++i)
                {
                    struct timeval current_time;
                    gettimeofday(&current_time, NULL);

                    if (!window[i].send ||
                        (!window[i].ack && current_time - window[i].sendTime > TIME_OUT))
                    {
                        /* fully send message */
                        // send(sock, window[i].buffPos, 0);
                        msgbuffer = filebuffer + window[i].buffPos * PKT_SIZE;
                        int msgSize;
                        if (bufferCount * PKT_SIZE > bufferSize)
                        {
                            msgSize = bufferSize - (bufferCount - 1) * PKTSIZE;
                        }
                        else
                        {
                            msgSize = PKTSIZE;
                        }
                        if (EOF && window[i].buffPos == bufferCount - 1)
                        {
                            /* send EOF in packet */
                            encode_send(windows[i].seqNum, sendbuffer, msgbuffer, 1, msgSize);
                        }
                        else
                        {
                            encode_send(windows[i].seqNum, sendbuffer, msgbuffer, 0, msgSize);
                        }
                    }
                }
            }
        }
    }
}