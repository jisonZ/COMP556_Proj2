#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <getopt.h>
#include "../utils.h"

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
    int i;

    char *recv_host = NULL;
    char *recv_port = NULL;
    char *subdir = NULL;
    char *filename = NULL;
    char whole_filename[100];
    struct timeval timeout;

    int opt;
    while ((opt = getopt(argc, argv, "r:f:")) != -1)
    {
        switch (opt)
        {
        case 'r':
            // Extract the receiver host and port from the -r argument
            recv_host = strtok(optarg, ":");
            recv_port = strtok(NULL, ":");
            break;
        case 'f':
            // Extract the filename and subdirectory from the -f argument
            strcpy(whole_filename, optarg);
            subdir = strtok(optarg, "/");
            filename = strtok(NULL, "/");
            break;
        default:
            printf("Usage: sendfile -r <recv host>:<recv port> -f <subdir>/<filename>\n");
            return 1;
        }
    }

    // Verify that all required arguments were provided
    if (recv_host == NULL || recv_port == NULL || subdir == NULL || filename == NULL)
    {
        printf("Usage: sendfile -r <recv host>:<recv port> -f <subdir>/<filename>\n");
        return 1;
    }

    // Print out the parsed arguments
    printf("Receiver host: %s\nReceiver port: %s\nSubdirectory: %s\nFilename: %s\nWhole FileName: %s\n", \ 
    recv_host,
           recv_port, subdir, filename, whole_filename);

    /* ------ identifying the server ------ */
    unsigned int server_addr;
    struct sockaddr_in sin, send_addr;

    struct addrinfo *getaddrinfo_result, hints;

    /* convert server domain name to IP address */
    memset(&hints, 0, sizeof(struct addrinfo));
    memset(&sin, 0, sizeof(sin));
    memset(&send_addr, 0, sizeof(send_addr));
    hints.ai_family = AF_INET; /* indicates we want IPv4 */

    if (getaddrinfo(recv_host, NULL, &hints, &getaddrinfo_result) == 0)
    {
        server_addr = (unsigned int)((struct sockaddr_in *)(getaddrinfo_result->ai_addr))->sin_addr.s_addr;
        freeaddrinfo(getaddrinfo_result);
    }

    /* ----- initialize and connect socket ------ */
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error opening UDP socket");
        abort();
    }
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = server_addr;
    sin.sin_port = htons(atoi(recv_port));


    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = INADDR_ANY; 
    send_addr.sin_port = htons(0);

    if (bind(sock, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0){
        perror("Error binding UDP socket");
        abort();
    }

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
    int SEND_LEN = PKT_SIZE + FNAME_LEN + DIRNAME_LEN + 16;
    char *sendbuffer = (char *)malloc(SEND_LEN);
    if (!sendbuffer)
    {
        perror("failed to allocate send buffer\n");
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
    // fd_set write_set;
    int max;

    struct timeval time_out;
    time_out.tv_usec = 1e5;
    time_out.tv_sec = 0;

    int select_retval;

    FILE *fp;
    fp = fopen(whole_filename, "r");

    int seq_num = 0;
    int largest_ack = -1;

    int eof = 0;

    socklen_t sin_addr_size;

    //TODO: timeout!
    while (!eof)
    {
        printf("New 1MB buffer\n");
        /* read from file to buffer */
        int bufferSize = fread(filebuffer, 1, MAX_BUFF * PKT_SIZE, fp);
        printf("bufferSize= %d\n",bufferSize);
        // printf("file buffer =  %s\n", filebuffer);

        int buff_pos = 0;

        // Is End of file?
        if (bufferSize < MAX_BUFF * PKT_SIZE)
        {
            eof = 1;
        }
        else
        {
            // fseek(fp, bufferSize, SEEK_CUR);
        }
        /* implement bufferSize/PKT_SIZE, EOF signal*/
        double t = (double) bufferSize / PKT_SIZE;
        printf("bufferCount= %f\n",t);
        int bufferCount = (int)ceil(t);
        printf("bufferCount= %d\n",bufferCount);

        seq_num = largest_ack+1;
        /* initialize window */
        for (i = 0; i < WINDOW_LEN; ++i)
        {
            struct packet p;
            p.seqNum = seq_num++;
            p.buffPos = buff_pos++;
            p.ack = 0;
            p.send = 0;
            window[i] = p;
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
                    int recvLen = recvfrom(sock, recvbuffer, RECV_LEN, MSG_WAITALL, (struct sock*)&sin, &sin_addr_size); // Ack
                    int ack_num;
                    int ack_status = decode_ACK(recvbuffer, recvLen, &ack_num);
			
                    int window_pos = ack_num - window[0].seqNum;
                    printf("ackNum: %i, ackStatus: %i\n", ack_num, ack_status);

                    if ((window_pos >= 0 && window_pos < WINDOW_LEN) && window[window_pos].ack == 0)
                    {
                        if (ack_status == -2)
                        {
                            window[window_pos].send = 0;
                        }
                        else if (ack_status == 0)
                        {
                            window[window_pos].ack = 1;
                            largest_ack = largest_ack > ack_num ? largest_ack : ack_num;
                        }
                    } else {
			printf("pkt: %i dropped\n");	
		    }
		    
                }
            }
            //print window
            printf("Before shift: ");
            for (i = 0; i < WINDOW_LEN; ++i) {
                printf("%i:%i:%i ", window[i].seqNum, window[i].ack, window[i].buffPos);
            }
            printf("\n");

            /* detect window shift */
            int shift = 0;
            for (i = 0; i < WINDOW_LEN; ++i)
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
                for (i = 0; i < WINDOW_LEN - shift; ++i)
                {
                    window[i] = window[i + shift];
                }

                /* reset rest of window */
                for (i = WINDOW_LEN - shift; i < WINDOW_LEN; ++i)
                {
                    struct packet p;
                    p.seqNum = seq_num++;
                    p.buffPos = buff_pos++;
                    p.ack = 0;
                    p.send = 0;
                    window[i] = p;
                }
            }
            
            // print window
            printf("After shift: ");
            for (i = 0; i < WINDOW_LEN; ++i) {
                printf("%i:%i:%i ", window[i].seqNum, window[i].ack, window[i].buffPos);
            }
            printf("\n");

            /* detect if all item been sent*/
            if (window[0].buffPos >= bufferCount)
            {
                break;
            }

            /* send frames */
            for (i = 0; i < WINDOW_LEN; ++i)
            {
                
                struct timeval current_time;
                gettimeofday(&current_time, NULL);
                timeval_subtract(&timeout, &current_time, &window[i].sendTime);
                
                if (window[i].buffPos >= bufferCount) {
                    break;
                }

                if (!window[i].send ||
                    (!window[i].ack && timeout.tv_sec > TIME_OUT))
                {
                    /* fully send message */
                    // send(sock, window[i].buffPos, 0);
                    
                    char* msgbuffer = filebuffer + window[i].buffPos * PKT_SIZE;
                    int msgSize;

                    if (window[i].buffPos >= bufferCount-1)
                    {
                        msgSize = bufferSize - (bufferCount - 1) * PKT_SIZE;
                    }
                    else
                    {
                        msgSize = PKT_SIZE;
                    }
                    // printf("%i, %i, %i\n", eof, window[i].buffPos, bufferCount);
                    if (eof && window[i].buffPos == bufferCount - 1)
                    {
                        /* send EOF in packet */
                        printf("send eof\n");
                        encode_send( sendbuffer, window[i].seqNum, 1, filename, subdir, msgbuffer, msgSize);
                    }
                    else
                    {
                        encode_send( sendbuffer, window[i].seqNum, 0, filename, subdir, msgbuffer, msgSize);
                    }
                    // int sendCount = send(sock, sendbuffer, msgSize+16, 0);
                    printf("msgSize: %i\n", msgSize);
                    int sendCount = sendto(sock, sendbuffer, 16+msgSize+FNAME_LEN+DIRNAME_LEN, 0, 
                                (const struct sockaddr *) &sin, sizeof(sin));
                    printf("[send data] start : %d\n", sendCount);
                    printf("send %i B\n", sendCount);
                    gettimeofday(&window[i].sendTime, NULL);
                }
            }
        }
        memset(filebuffer, 0, MAX_BUFF*PKT_SIZE);
    }
    free(recvbuffer);
    free(sendbuffer);
    free(filebuffer);
    fclose(fp);
}
