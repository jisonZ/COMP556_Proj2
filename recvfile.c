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
#define ACK_LEN 12
#include "utils.c"

struct packet {
    int seq_num;
    int buffPos;
    int ack;
    int error;
};

int main(int argc, char **argv)
{
    struct sockaddr_in sin, addr;

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
    sock_time_out.tv_usec = 1e5;
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
    struct packet* window = (struct packet *)malloc(WINDOW_LEN);
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
    time_out.tv_usec = 1e5;
    time_out.tv_sec = 0;

    int lSeqNum = 0;
    int lBuffPos = 0;

    while (!recv_done) {
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
                    int recvLen = recv(sock, recvbuffer, RECV_LEN, 0);
                    int seq_num;
                    int msg_size;
                    int recv_done = decode_send(recvbuffer, recvLen, &seq_num, msgbuffer, &msg_size);
                    if (recv_done != -1){ /* This means we have received a packet with garbage length, just ignored it*/
                        int error_bit = (recv_done == -2 ? 1 : 0);
                        if (seq_num >= window[0].seq_num && seq_num < window[0].seq_num+WINDOW_LEN) {
                            int curWindowIdx = seq_num - window[0].seq_num;
                            window[curWindowIdx].ack = 1;
                            window[curWindowIdx].error = error_bit;


                            // MSG field = msg  + checksum
                            // we need to split it with checksum
                            char * msg_to_write = (char *)malloc(msg_size);
                            strncpy(msg_to_write, msgbuffer, msg_size);
                            if (strlen(filebuffer) >= MAX_BUFF * PKT_SIZE) {
                                size_t writen_size = fwrite(filebuffer, 1, MAX_BUFF * PKT_SIZE, fp);
                                fseek(fp, writen_size, SEEK_CUR);
                                memset(filebuffer, 0, writen_size);
                            }
                            *(filebuffer + window[curWindowIdx].buffPos*PKT_SIZE) = *msg_to_write;
                            
                            free(msg_to_write);
                        } else if (seq_num < window[0].seq_num) {
                            /* TODO: send ack */
                            // Send it in case send file always send
                            encode_ACK(seq_num, error_bit, ackbuffer);
                            send(sock, ackbuffer, ACK_LEN, 0);
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

            // send ack
            for (int i = 0; i < WINDOW_LEN; ++i) {
                if (!window[i].ack) {
                    /* send ack*/
                    encode_ACK(window[i].seq_num, window[i].error, ackbuffer);
                    send(sock, ackbuffer, ACK_LEN, 0);
                }
            }
            if (recv_done == 1){
                return 0;
            }
        }
    }



    // while (1)
    // {
    //     int recv_packet_size = recvfrom(sock, recv_buffer, PKT_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
    //     if (recv_packet_size < 0)
    //     {
    //         break;
    //     }

    //     int eof = decode_send(recv_buffer, recv_packet_size, &seq_num, msg, &msg_size);

    //     // corrupt packet
    //     if (eof == -1)
    //     {
    //         printf("[recv corrupt packet]\n");
    //         free(recv_buffer);

    //         // still send ack, set error flag
    //         encode_ACK(seq_num, 1, ack_buffer);
    //         int send_packet_size = sendto(sock, ack_buffer, 12, 0, (const struct sockaddr *)&addr, sizeof(addr));
    //         if (send_packet_size < 0)
    //         {
    //             printf("send error \n");
    //             abort();
    //         }
    //         continue;
    //     }

    //     int recv_seq_num = recv_buffer[??];
    //     msg_size = ? ? ;

    //     if (recv_seq_num == seq_num + 1)  
    //     { // in order packet
    //         printf("[recv data] %d %d ACCEPTED(in-order)\n", total_data_size, msg_size);
    //         seq_num = recv_seq_num;
    //         total_data_size += msg_size;
    //     } // duplicate packet
    //     else if (recv_seq_num == seq_num)
    //     {   
    //         printf("[recv data] %d %d IGNORED\n", total_data_size, msg_size);
    //         free(recv_buffer);

    //     } // out of order packet
    //     else
    //     {
    //         printf("[recv data] %d %d ACCEPTED(out-of-order)\n", total_data_size, msg_size);
    //         // TODO: handle out of order packet
    //     }

    // }
        

    fclose(fp);
    free(recvbuffer);
    free(ackbuffer);
    free(filebuffer);
    free(msgbuffer);
    printf("[completed]\n");
    return 0;
}