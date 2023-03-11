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
#include <sys/stat.h>
#include <getopt.h>
#include "../utils.h"

struct packet {
    int seq_num;
    int buffPos;
    int ack;
    int error;
};

int main(int argc, char **argv)
{
    int i;
    struct sockaddr_in sin, send_addr;

    int recv_port = -1;

    // char* filename = "tempfile";
    // FILE* fp = fopen(filename, "w");

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

    /* ----- allocate buffer ----- */
    char* filebuffer = (char *)malloc(MAX_BUFF * PKT_SIZE);
    if (!filebuffer) {
        perror("failed to allocate file buffer\n");
        abort();
    }
    int RECV_LEN = PKT_SIZE+FNAME_LEN+DIRNAME_LEN+16;
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

    // int seq_num;
    // int msg_size;

    int recv_done = 0;

    fd_set read_set;
    int max;
    struct timeval time_out;
    time_out.tv_usec = 0;
    time_out.tv_sec = 0;

    int lSeqNum = 0;
    int recvDoneSeq = -1;

    int largestAck_num = -1;
    socklen_t send_addr_len;
    int totalpktCount = 0;
    char subdir[DIRNAME_LEN];
    char filename[FNAME_LEN];
    int isFileCreated = 0;
    FILE *fp;
    int num_of_filebuffer_have_writen;
    // printf("Begin to receive data\n");

    //TODO: timeout
    while (recv_done != 1) {
        int lBuffPos = 0;
        int buffSize = 0;
        lSeqNum = largestAck_num+1;

        // initialize window
        for (i = 0; i < WINDOW_LEN; ++i) {
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
                    
                    int seq_num;
                    int msg_size;
                    recv_done = decode_send(recvbuffer, recvLen, &seq_num, msgbuffer, &msg_size, subdir, filename);
                    
                    if (recv_done == 1) {
                        recvDoneSeq = seq_num;
                    }else if (recv_done == -1 || recv_done == -2){
                        printf("[recv corrupt packet]\n");
                    }

                    // printf("recving %i Msg Byte\n", msg_size);
                    // printf("recv_done:%i, seqnum:%i\n", recv_done, seq_num);

                    if (recv_done >= 0){ /* This means we have received a packet with garbage length, just ignored it*/
                        
                        int error_bit = (recv_done == -2 ? 1 : 0);
                        // printf("WINDOW CHECK: seq: %i, error: %i, recv_done: %i\n", seq_num, error_bit, recv_done);

                        if (seq_num >= window[0].seq_num && seq_num < window[0].seq_num+WINDOW_LEN && window[seq_num-window[0].seq_num].ack == 0) {
                            
                            // printf("[recv data] %d ACCPETED. ", recvLen);
                            int curWindowIdx = seq_num - window[0].seq_num;

                            // send ACK 
                            //printf("seq: %i, error: %i, recv_done: %i\n", window[curWindowIdx].seq_num, error_bit, recv_done);

                            encode_ACK(window[curWindowIdx].seq_num, error_bit, ackbuffer);
                            sendto(sock, ackbuffer, SEND_LEN, 0, (const struct sockaddr *)&send_addr, send_addr_len);

                            if (error_bit == 0) {
                                window[curWindowIdx].ack = 1;
                                largestAck_num++;

                                struct stat st;
                                memset(&st, 0, sizeof(struct stat));
                                
                                if (stat(subdir, &st) == -1) {
                                    char *dir;
                                    printf("-- Creating new directory %s\n", subdir);
                                    dir = strdup(subdir);
                                    mkdir(dir, 0777);
                                    free(dir);
                                }

                                if (!isFileCreated) {
                                    printf("-- Creating file %s\n", filename);
                                    char * target_file_name;
                                    target_file_name = strdup(subdir);
                                    strcat(target_file_name, "/");
                                    strcat(target_file_name, filename);
                                    strcat(target_file_name, ".recv");
                                    fp = fopen(target_file_name, "w");
                                    isFileCreated = 1;
                                    free(target_file_name);
                                }
                                int is_ordered = seq_num == window[0].seq_num;
                                if (is_ordered)
                                    printf("[recv data] %d %d ACCEPTED - in-order\n", num_of_filebuffer_have_writen * 1024000 + window[curWindowIdx].buffPos * PKT_SIZE, msg_size);
                                else
                                    printf("[recv data] %d %d ACCEPTED - out-of-order\n", num_of_filebuffer_have_writen * 1024000 + window[curWindowIdx].buffPos * PKT_SIZE, msg_size);
                                memcpy(filebuffer+window[curWindowIdx].buffPos*PKT_SIZE, msgbuffer, msg_size);                 
                                buffSize += msg_size;
                                totalpktCount++;

                            }

                            

                        } else if ((seq_num < window[0].seq_num && seq_num >= 0)) {
                            // Send it in case send file always send
                            // printf("[recv data] start %d IGNORED. \n", recvLen);
                            printf("[recv data] %d %d IGNORED\n", num_of_filebuffer_have_writen * 1024000, msg_size);
                            encode_ACK(seq_num, error_bit, ackbuffer);
                            sendto(sock, ackbuffer, SEND_LEN, 0, (const struct sockaddr *)&send_addr, send_addr_len);
                            
                        }else{
                            printf("[recv data] %d %d IGNORED\n", num_of_filebuffer_have_writen * 1024000, msg_size);
                        }
                    }
                }
            }

            // shift window
            int shift = 0;
            for (i = 0; i < WINDOW_LEN; ++i) {
                if (window[i].ack) {
                    shift++;
                } else {
                    break;
                }
            }

            // shift not ack item to the front
            
            for (i = shift; i < WINDOW_LEN; ++i) {
                window[i-shift] = window[i];
            }

            for (i = WINDOW_LEN-shift; i < WINDOW_LEN; ++i) {
                window[i].seq_num = lSeqNum++;
                window[i].buffPos = lBuffPos++;
                window[i].ack = 0;
                window[i].error = 0;
            }

            //print window
            // for (i = 0; i < WINDOW_LEN; ++i) {
            //     printf("%i ", window[i].seq_num);
            // }
            // printf("\n");

            // if EOF: break if window[0].buffPos >= EOF position
            // if not EOF: break if window[0].buffPos >= MAX_LEN
            if (recv_done) {
                if (recvDoneSeq != -1 && window[0].seq_num > recvDoneSeq) {
                    break;
                }
            } else {
                if (window[0].buffPos >= MAX_BUFF) {
                    break;
                }
            }
        }
        // printf("done write to file: %i %s\n", recv_done, filebuffer);
        // write to file
        // printf("Write to file: %i\n", buffSize);
        size_t writen_size = fwrite(filebuffer, 1, buffSize, fp);
        // fseek(fp, writen_size, SEEK_CUR);
        memset(filebuffer, 0, writen_size);
        num_of_filebuffer_have_writen ++;
                            
    } // While Recv Done

    // print total recived file length
    //fseek(fp, 0L, SEEK_END);
    //int sz = ftell(fp);
    // printf("written size %d\n", sz);
    // printf("written pkt %d\n", totalpktCount);
    struct timeval begin_time, end_time, result;
    gettimeofday(&begin_time, NULL);
    // Timeout with ACK
    while (1) {
        // initialize select()
        gettimeofday(&end_time, NULL);
        timeval_subtract(&result, &end_time, &begin_time);
        if (result.tv_sec > 2){
            // timeout
            break;
        }
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
                gettimeofday(&begin_time, NULL);

                int seq_num;
                int msg_size;
                recv_done = decode_send(recvbuffer, recvLen, &seq_num, msgbuffer, &msg_size, subdir, filename);

                
                encode_ACK(seq_num, 0, ackbuffer);
                sendto(sock, ackbuffer, SEND_LEN, 0, (const struct sockaddr *)&send_addr, send_addr_len);
                //printf("Received Last Data Packet");
            }
        }
    }
    
    fclose(fp);
    free(recvbuffer);
    free(ackbuffer);
    free(filebuffer);
    free(msgbuffer);
    printf("[completed]\n");
    return 0;
    /* TODO: timeout at the end*/
}