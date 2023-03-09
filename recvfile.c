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
#include <sys/time.h>
#include <sys/select.h>
#include "utils.h"

struct packet {
    int seq_num;
    int buffPos;
    int ack;
    int error;
};

int main(int argc, char **argv)
{
    struct sockaddr_in sin, send_addr;

    int recv_port = -1;

    char* filename = "tempfile";
    FILE* fp = fopen(filename, "w");

    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                // Extract the receiver port from the -p argument
                recv_port = atoi(optarg);
                break;
            default:
                printf("Usage: recvfile -p <recv port>\n");
                return 1;
        }
    }

    // Verify that the required argument was provided
    if (recv_port == -1) {
        printf("Usage: recvfile -p <recv port>\n");
        return 1;
    }

    int sock;
    // int optval = 1;

    /* socket address variables for a connected client */
    socklen_t addr_len = sizeof(struct sockaddr_in);


    /* create a server socket to listen for TCP connection requests */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("opening UDP socket");
        abort();
    }

    /* fill in the address of the server socket */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(recv_port);

    /* bind server socket to the address */
    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("binding socket to address");
        abort();
    }

    struct timeval sock_time_out;
    sock_time_out.tv_usec = 0;
    sock_time_out.tv_sec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&sock_time_out, sizeof sock_time_out);

    //FILE *fp;
    long file_size;  // TODO: extract file size?

    /* ----- allocate buffer ----- */
    char* filebuffer = (char *)malloc(MAX_BUFF * PKT_SIZE);
    if (!filebuffer) {
        perror("failed to allocate file buffer\n");
        abort();
    }
    int RECV_LEN = PKT_SIZE+16;
    char *recvbuffer = (char *)malloc(RECV_LEN);
    if (!recvbuffer)
    {
        perror("failed to allocate recv buffer\n");
        abort();
    }
    int SEND_LEN = 12;
    char *ackbuffer = (char *)malloc(SEND_LEN);
    if (!ackbuffer)
    {
        perror("failed to allocate send buffer\n");
        abort();
    }

    char *msgbuffer = (char *)malloc(PKT_SIZE);
    if (!msgbuffer) {
        perror("failed to allocate msg buffer\n");
        abort();
    }

    /* ----- allocate window ----- */
    struct packet* window = (struct packet *)malloc(WINDOW_LEN*sizeof(struct packet));
    if (!window) {
        perror("failed to allocate window\n");
        abort();
    }

    int seq_num;
    int msg_size;
    int total_data_size = 0;   

    int recv_done = 0;

    fd_set read_set;
    int max;
    struct timeval time_out;
    time_out.tv_usec = 0;
    time_out.tv_sec = 0;

    int lSeqNum = 0;
    socklen_t send_addr_len;

    while (recv_done != 1) {
        int lBuffPos = 0;
        int buffSize = 0;

        // initialize window
        for (int i = 0; i < WINDOW_LEN; ++i) {
            struct packet p;
            p.seq_num = lSeqNum++;
            p.buffPos = lBuffPos++;
            p.ack = 0;
            p.error = 0;
            window[i] = p;
        }
        
        while (1) {
            // initialize select()
            FD_ZERO(&read_set);
            FD_SET(sock, &read_set);
            max = sock;
            int select_retval = select(max+1, &read_set, NULL, NULL, &time_out);
            if (select_retval < 0) {
                perror("select failed\n");
                abort();
            }

            // recv and store packet 
            if (select_retval > 0) {
                // TODO: delete this
                if (FD_ISSET(sock, &read_set)) {
                    //int recvLen = recv(sock, recvbuffer, RECV_LEN, 0);
                    int recvLen = recvfrom(sock, (char *)recvbuffer, RECV_LEN, 
                    MSG_WAITALL, (struct sockaddr *) &send_addr, &send_addr_len);
                    // printf("recving %i Byte\n", recvLen);

                    int seq_num;
                    int msg_size;
                    recv_done = decode_send(recvbuffer, recvLen, &seq_num, msgbuffer, &msg_size);
                    
                    if (recv_done != -1){ /* This means we have received a packet with garbage length, just ignored it*/
                        
                        int error_bit = (recv_done == -2 ? 1 : 0);
                        if (seq_num >= window[0].seq_num && seq_num < window[0].seq_num+WINDOW_LEN) {
                            int curWindowIdx = seq_num - window[0].seq_num;

                            // send ACK 
                            printf("seq: %i, error: %i, eof: %i\n", window[curWindowIdx].seq_num, error_bit, recv_done);
                            encode_ACK(window[curWindowIdx].seq_num, error_bit, ackbuffer);
                            sendto(sock, ackbuffer, SEND_LEN, 0, (const struct sockaddr *)&send_addr, send_addr_len);

                            if (!error_bit) {
                                window[curWindowIdx].ack = 1;
                            }
                            // window[curWindowIdx].error = error_bit;
                            
                            // MSG field = msg  + checksum
                            // we need to split it with checksum
                            char * msg_to_write = (char *)malloc(msg_size);
                            memcpy(msg_to_write, msgbuffer, msg_size);
                            strncpy(filebuffer+window[curWindowIdx].buffPos*PKT_SIZE, msgbuffer, msg_size);                 
                            buffSize += msg_size;
                            printf("File Buffer is %s\n", msg_to_write);
                            // if EOF recived, break
                            if (recv_done == 1 || buffSize >= MAX_BUFF*PKT_SIZE) {
                                printf("recv done: %i, buffSize: %i\n", recv_done, buffSize);
                                break;
                            }

                        } else if (seq_num < window[0].seq_num) {
                            // Send it in case send file always send
                            encode_ACK(seq_num, error_bit, ackbuffer);
                            sendto(sock, ackbuffer, SEND_LEN, 0, (const struct sockaddr *)&send_addr, send_addr_len);
                        }
                    }  
                }
            }

            // shift window
            int shift = 0;
            for (int i = 0; i < WINDOW_LEN; ++i) {
                if (window[i].ack) {
                    shift++;
                } else {
                    break;
                }
            }

            // shift not ack item to the front
            for (int i = shift; i < WINDOW_LEN; ++i) {
                window[i-shift] = window[i];
            }
            for (int i = WINDOW_LEN-shift; i < WINDOW_LEN; ++i) {
                window[i].seq_num = lSeqNum++;
                window[i].buffPos = lBuffPos++;
                window[i].ack = 0;
                window[i].error = 0;
            }
        }
        printf("done write to file: %i %s\n", recv_done, filebuffer);
        // write to file
        size_t writen_size = fwrite(filebuffer, 1, buffSize, fp);
        fseek(fp, writen_size, SEEK_CUR);
        memset(filebuffer, 0, writen_size);
                            
    } // While Recv Done
    fclose(fp);
    free(recvbuffer);
    free(ackbuffer);
    free(filebuffer);
    free(msgbuffer);
    printf("[completed]\n");
    return 0;
    /* TODO: timeout at the end*/
}