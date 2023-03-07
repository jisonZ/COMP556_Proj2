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

#define MAX_PACKET_SIZE 1024
#define WINDOW_SIZE 5

int main(int argc, char **argv)
{
    struct sockaddr_in sin, addr;
    unsigned short server_port;

    if (argc == 3)
    {
        server_port = atoi(argv[2]);
    }
    else
    {
        perror("usage: ./recvfile -p <recv port>\n");
        abort();
    }

    int sock, max;
    // int optval = 1;

    /* socket address variables for a connected client */
    socklen_t addr_len = sizeof(struct sockaddr_in);

    /* maximum number of pending connection requests */
    // int BACKLOG = 5;

    /* a buffer to read data */
    // char *buf;
    // int BUF_LEN = MaxBufSiz;

    // buf = (char *)malloc(BUF_LEN);

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
    sin.sin_port = htons(server_port);

    /* bind server socket to the address */
    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("binding socket to address");
        abort();
    }

    /* variables for select */
    // fp_set read_set, write_set;
    struct timeval time_out;
    // int select_retval;

    time_out.tv_sec = 1;
    time_out.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&time_out, sizeof time_out);

    FILE *fp;
    long file_size;  // TODO: extract file size?

    char ack_buffer[12];
    char *recv_buffer;
    recv_buffer = (char *)malloc(PKT_SIZE);
    int seq_num = 1;
    int msg_size;
    char *msg = malloc(MAX_MSG_SIZE);
    int total_data_size = 0;

    while (1)
    {
        int recv_packet_size = recvfrom(sock, recv_buffer, PKT_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
        if (recv_packet_size < 0)
        {
            break;
        }

        int eof = decode_send(recv_buffer, recv_packet_size, &seq_num, msg, &msg_size);

        // corrupt packet
        if (eof == -1)
        {
            printf("[recv corrupt packet]\n");
            free(recv_buffer);

            // still send ack, set error flag
            encode_ACK(seq_num, 1, ack_buffer);
            int send_packet_size = sendto(sock, ack_buffer, 12, 0, (const struct sockaddr *)&addr, sizeof(addr));
            if (send_packet_size < 0)
            {
                printf("send error \n");
                abort();
            }
            continue;
        }

        int recv_seq_num = recv_buffer[??];
        msg_size = ? ? ;

        if (recv_seq_num == seq_num + 1)  
        { // in order packet
            printf("[recv data] %d %d ACCEPTED(in-order)\n", total_data_size, msg_size);
            seq_num = recv_seq_num;
            total_data_size += msg_size;
        } // duplicate packet
        else if (recv_seq_num == seq_num)
        {   
            printf("[recv data] %d %d IGNORED\n", total_data_size, msg_size);
            free(recv_buffer);
            seq_num = recv_seq_num;

        } // out of order packet
        else
        {
            printf("[recv data] %d %d ACCEPTED(out-of-order)\n", total_data_size, msg_size);
            // TODO: handle out of order packet
        }

        

    fclose(fp);
    free(recv_buffer);
    printf("[completed]\n");
    return 0;
}