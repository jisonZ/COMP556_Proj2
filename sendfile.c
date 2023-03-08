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
#define PKT_SIZE 1024

#include "utils.c"

struct packet
{
    int seqNum;
    int buffPos;
    int ack;
    int send;
    struct timeval sendTime;
};

int main(int argc, char **argv)
{

    char *recv_host = NULL;
    char *server_port = NULL;
    char *subdir = NULL;
    char *filename = NULL;
    char whole_filename[100];

    int opt;
    while ((opt = getopt(argc, argv, "r:f:")) != -1) {
        switch (opt) {
            case 'r':
                // Extract the receiver host and port from the -r argument
                recv_host = strtok(optarg, ":");
                server_port = strtok(NULL, ":");
                break;
            case 'f':
                // Extract the filename and subdirectory from the -f argument
                subdir = strtok(optarg, "/");
                filename = strtok(NULL, "/");
                break;
            default:
                printf("Usage: sendfile -r <recv host>:<recv port> -f <subdir>/<filename>\n");
                return 1;
        }
    }

    // Verify that all required arguments were provided
    if (recv_host == NULL || server_port == NULL || subdir == NULL || filename == NULL) {
        printf("Usage: sendfile -r <recv host>:<recv port> -f <subdir>/<filename>\n");
        return 1;
    }

    // Print out the parsed arguments
    printf("Receiver host: %s\nReceiver port: %s\nSubdirectory: %s\nFilename: %s\n", recv_host, server_port, subdir, filename);



 




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
    if (!filebuffer)
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

    struct packet *window = (struct packet *)malloc(WINDOW_LEN * sizeof(struct packet));
    if (!window)
    {
        perror("fail to allocate window buffer\n");
        abort();
    }


    /* ----- send file ----- */
    fd_set read_set;
    fd_set write_set;
    int max;

    struct timeval time_out;
    time_out.tv_usec = 1e5;
    time_out.tv_sec = 0;

    int select_retval;

    FILE *fp;
    fp = fopen(whole_filename, "r");

    int seq_num = 0;

    int eof = 0;

    int buff_pos = 0;
        
    for (int i = 0; i < WINDOW_LEN; ++i)
    {
        struct packet p;
        p.seqNum = seq_num;
        p.buffPos = buff_pos;
        p.ack = 0;
        p.send = 0;
        window[i] = p;
        seq_num++;
        buff_pos++;
    }
    
    while (!eof)
    {
        /* read from file to buffer */
        int bufferSize = fread(filebuffer, 1, MAX_BUFF * PKT_SIZE, fp);

        // Is End of file?
        if (bufferSize < MAX_BUFF * PKT_SIZE)
        {
            eof = 1;
        }else{
            fseek(fp, bufferSize, SEEK_CUR);
        }
        /* implement bufferSize/PKT_SIZE, EOF signal*/
        int bufferCount = bufferSize /= PKT_SIZE;

        /* initialize window */
        

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
            select_retval = select(max + 1, &read_set, NULL, NULL, &time_out);
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
                            window[window_pos].send = 0;
                        }
                        else if (ack_status == 0)
                        {
                            window[ack_num - first_window_seq].ack = 1;
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
                        ++shift;
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
                            msgSize = bufferSize - (bufferCount - 1) * PKT_SIZE;
                        }
                        else
                        {
                            msgSize = PKT_SIZE;
                        }
                        if (eof && window[i].buffPos == bufferCount - 1)
                        {
                            /* send EOF in packet */
                            encode_send(window[i].seqNum, sendbuffer, msgbuffer, 1, msgSize);
                        }
                        else
                        {
                            encode_send(window[i].seqNum, sendbuffer, msgbuffer, 0, msgSize);
                        }
                        gettimeofday(&window[i].sendTime, NULL);
                    }
                }
            }
        }
        free(whole_filename);
        free(recvbuffer);
        free(sendbuffer);
        free(filebuffer);
        free(msgbuffer);
        free(window);
        fclose(fp);
    }
}