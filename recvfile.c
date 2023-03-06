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

typedef struct
{
    int seq_num;
    int length;
    char data[MAX_PACKET_SIZE];
} packet;

// TODO: send ack
void send_ack(int seq_num, int sock, struct sockaddr_in addr) {

}

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

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof time_out);

    int packet_size; // TODO: packet size=??
    int expected_seq_num = 0;
    int window_base = 0;
    int window_end = WINDOW_SIZE - 1;
    int acked[WINDOW_SIZE];
    memset(acked, 0, sizeof(acked));

    char ack[2];
    ack[0] = 1;
    int ack_size;  // TODO: ack size =??
    char* ack_packet = malloc(ack_size);
    char* recv_buffer;
    char last_seq_num = (char) 1;
    int total_data = 0;
    FILE *fp;
    long file_size;  // TODO: extract file size?

    while (1) {
        recv_buffer = (char*) malloc(packet_size);
        int count = recvfrom(sock, recv_buffer, packet_size, 0, (struct sockaddr*)&addr, &addr_len);
        if (count < 0)
            break;

        // TODO: checksum
        unsigned int checksum;
        unsigned int checksum_sent;
        // if checksum mismatch -> corrupt packet
        if (checksum != checksum_sent) {
            printf("[recv corrupt packet]\n");
            free(recv_buffer);
            continue;
        }

        // extract sequence number
        int seq_num = (int)(recv_buffer[1]);

        int data_size;  // TODO: data_size

        if (seq_num < expected_seq_num || seq_num > expected_seq_num + WINDOW_SIZE - 1) {
            // out of window
            printf("[recv data] %d %u IGNORED\n", total_data, data_size);
            // send duplicate ACK
            send_ack(last_seq_num, sock, addr);
            free(recv_buffer);
            continue;
        }

        int window_index = seq_num - expected_seq_num;

        if (acked[window_index] == 0) {
            // valid packet received
            printf("[recv data] %d %u ACCEPTED(in-order)\n", total_data, data_size);

            char file_path[256];  
            
            // TODO: extract file_path

            fp = fopen(file_path, 'w');

            // write to file
            fwrite(recv_buffer, 1, data_size, fp);
            total_data += data_size;

            acked[window_index] = 1;

            // slide window
            while (acked[0] == 1) {
                window_base++;
                window_end++;
                expected_seq_num++;

                // mark packet as unack'ed
                acked[0] = 0;

                // Check if file transfer is complete
                if (total_data == file_size) {
                    break;
                }
            }
        } else {
            // duplicate packet
            printf("[recv data] %d %u IGNORED\n", total_data, data_size);

            // TODO: what about out-of-order packet?
        }
        // send ACK
        send_ack(seq_num, sock, addr);

        if (total_data == file_size) {
            break;
        }
    }
    fclose(fp);
    free(recv_buffer);
    printf("[completed]\n");
    return 0;
}