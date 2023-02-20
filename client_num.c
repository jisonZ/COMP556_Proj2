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

#define htonll(x) ((1 == htonl(1)) ? (x) : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))

/* simple client, takes two parameters, the server domain name,
   and the server port number */

unsigned char *gen_rdm_bytestream(size_t num_bytes)
{
  unsigned char *stream = malloc(num_bytes);
  size_t i;

  for (i = 0; i < num_bytes; i++)
  {
    stream[i] = rand();
  }

  return stream;
}

int main(int argc, char **argv)
{

  /* our client socket */
  int sock;

  /* variables for identifying the server */
  unsigned int server_addr;
  struct sockaddr_in sin;
  struct addrinfo *getaddrinfo_result, hints;

  /* convert server domain name to IP address */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; /* indicates we want IPv4 */

  if (getaddrinfo(argv[1], NULL, &hints, &getaddrinfo_result) == 0)
  {
    server_addr = (unsigned int)((struct sockaddr_in *)(getaddrinfo_result->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(getaddrinfo_result);
  }

  /* server port number */
  unsigned short server_port = atoi(argv[2]);
  /* size of the message */
  /* 18 <= size <= 65535*/
  int msgSize = atoi(argv[3]);
  if (msgSize < 18 || msgSize > 65535)
  {
    perror("out of bound message size");
    abort();
  }
  /* Count number of message exchange*/
  int count = atoi(argv[4]);

  char *buffer, *sendbuffer;

  /* allocate a memory buffer in the heap */
  /* putting a buffer on the stack like:

         char buffer[500];

     leaves the potential for
     buffer overflow vulnerability */
  buffer = (char *)malloc(msgSize);
  if (!buffer)
  {
    perror("failed to allocated buffer");
    abort();
  }

  sendbuffer = (char *)malloc(msgSize);
  if (!sendbuffer)
  {
    perror("failed to allocated sendbuffer");
    abort();
  }

  /* create a socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    perror("opening TCP socket");
    abort();
  }

  /* fill in the server's address */
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = server_addr;
  sin.sin_port = htons(server_port);
  printf("Client is goint to Connect\n");
  /* connect to the server */
  if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    perror("connect to server failed");
    abort();
  }
  printf("Client is Connected\n");

  /* initialize file for logging*/
  FILE *fp;

  fp = fopen("Part3.log", "w+");
  fprintf(fp, "%d\n", count);

  struct timeval current_time;
  /* send messgae in a loop*/
  size_t i;
  for (i = 0; i < count; ++i)
  {
    printf("PrepareSend Message in %d times\n", i);

    /* first byte of the sendbuffer is used to describe the number of
       bytes used to encode a number, the number value follows the first
       byte */

    // tv_sec (8B) + tv_usec (8B)
    // use short (2B) for size
    *(short *)sendbuffer = htons(msgSize);
    long long *timestampPtr = (long long *)((short *)sendbuffer + 1);

    // add timestamp to buffer
    gettimeofday(&current_time, NULL);
    *timestampPtr = htonll((long long)current_time.tv_sec);
    *(timestampPtr + 1) = htonll((long long)current_time.tv_usec);
    unsigned char *dataPtr = (unsigned char *)(timestampPtr + 2);

    // generate random data array
    unsigned char *testbytestream = gen_rdm_bytestream(msgSize - 18);
    *dataPtr = testbytestream;
    printf("send message at tv_sec %lld, tv_usec %lld\n", current_time.tv_sec, current_time.tv_usec);
    fprintf(fp, "%lld.%lld\n", current_time.tv_sec, current_time.tv_usec);

    int sendMsgSize = 0;
    // int temp = send(sock, sendbuffer + sendMsgSize, msgSize-sendMsgSize, 0);
    
    while (sendMsgSize < msgSize) {
      int temp = send(sock, sendbuffer + sendMsgSize, msgSize-sendMsgSize, 0);
      // printf("temp = %d\n", temp);
      sendMsgSize += temp;
    }

    // wait till the response
    /* everything looks good, since we are expecting a
   message from the server in this example, let's try receiving a
   message from the socket. this call will block until some data
   has been received */
    char *recvBuffer;
    recvBuffer = (char *)malloc(msgSize);
    int recvCount = 0;

    while (recvCount < msgSize)
    {
      int tmp = recv(sock, buffer, msgSize, 0);
      if (tmp < 0)
      {
        perror("receive failure");
        abort();
      }
      *(recvBuffer + recvCount) = *buffer;
      recvCount += tmp;
    }
    
    /* get timestamp at receiving time*/
    gettimeofday(&current_time, NULL);

    // generate random data array
    fprintf(fp, "%lld.%lld\n", current_time.tv_sec, current_time.tv_usec);

    /* we compare the recieved message with sent message*/
    if (*(unsigned char *)recvBuffer == *(unsigned char *)sendbuffer)
    {
      printf("Return Message Successfully decoded!\n");
    }
    else
    {
      printf("Return Message Corrupted!\n");
    }
  }
  fclose(fp);

  close(sock);
  free(buffer);
  free(sendbuffer);
  return 0;
}
