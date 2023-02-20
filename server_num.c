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

int MaxBufSiz = 65535;
/**************************************************/
/* a few simple linked list functions             */
/**************************************************/
#define htonll(x) ((1 == htonl(1)) ? (x) : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))

/* A linked list node data structure to maintain application
   information related to a connected socket */
struct node
{
  int socket;
  struct sockaddr_in client_addr;

  int pending_data; /* flag to indicate whether there is more data to send */
  char *sendBuf;    /* buffer for the remaining send message*/
  int sendLen;      /* number of bytes to send*/

  int pending_rece; /* flag to indicate whether more data need to be received*/
  char *receBuf;    /* buffer for the received message*/
  int receLen;      /* received byte length*/
  int expectedRece; /* expected bytes to receive*/

  struct node *next;
};

/* remove the data structure associated with a connected socket
   used when tearing down the connection */
void dump(struct node *head, int socket)
{
  struct node *current, *temp;

  current = head;

  while (current->next)
  {
    if (current->next->socket == socket)
    {
      /* remove */
      temp = current->next;
      current->next = temp->next;
      //free(temp->sendBuf);
      free(temp->receBuf);
      free(temp); /* don't forget to free memory */
      return;
    }
    else
    {
      current = current->next;
    }
  }
}

/* create the data structure associated with a connected socket */
void add(struct node *head, int socket, struct sockaddr_in addr)
{
  struct node *new_node;

  new_node = (struct node *)malloc(sizeof(struct node));
  new_node->socket = socket;
  new_node->client_addr = addr;

  new_node->pending_data = 0;
  new_node->pending_rece = 0;
  // new_node->sendBuf = (char *)malloc(MaxBufSiz);
  new_node->receBuf = (char *)malloc(MaxBufSiz);
  new_node->sendLen = 0;
  new_node->receLen = 0;
  new_node->expectedRece = -1;

  new_node->next = head->next;
  head->next = new_node;
}

/*****************************************/
/* main program                          */
/*****************************************/

/* simple server, takes one parameter, the server port number */
int main(int argc, char **argv)
{

  /* socket and option variables */
  int sock, new_sock, max;
  int optval = 1;

  /* server socket address variables */
  struct sockaddr_in sin, addr;
  unsigned short server_port = atoi(argv[1]);

  /* socket address variables for a connected client */
  socklen_t addr_len = sizeof(struct sockaddr_in);

  /* maximum number of pending connection requests */
  int BACKLOG = 5;

  /* variables for select */
  fd_set read_set, write_set;
  struct timeval time_out;
  int select_retval;

  /* a silly message */
  // char *message = "Welcome! COMP/ELEC 429 Students!\n";

  /* number of bytes sent/received */
  int count;

  /* numeric value received */
  // int num;

  /* linked list for keeping track of connected sockets */
  struct node head;
  struct node *current, *next;

  /* a buffer to read data */
  char *buf;
  int BUF_LEN = MaxBufSiz;

  buf = (char *)malloc(BUF_LEN);

  /* initialize dummy head node of linked list */
  head.socket = -1;
  head.next = 0;

  /* create a server socket to listen for TCP connection requests */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    perror("opening TCP socket");
    abort();
  }

  /* set option so we can reuse the port number quickly after a restart */
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
  {
    perror("setting TCP socket option");
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

  /* put the server socket in listen mode */
  if (listen(sock, BACKLOG) < 0)
  {
    perror("listen on socket failed");
    abort();
  }

  /* now we keep waiting for incoming connections,
     check for incoming data to receive,
     check for ready socket to send more data */
  while (1)
  {
    /* set up the file descriptor bit map that select should be watching */
    FD_ZERO(&read_set);  /* clear everything */
    FD_ZERO(&write_set); /* clear everything */

    FD_SET(sock, &read_set); /* put the listening socket in */
    max = sock;              /* initialize max */

    /* put connected sockets into the read and write sets to monitor them */
    for (current = head.next; current; current = current->next)
    {
      FD_SET(current->socket, &read_set);

      if (current->pending_data)
      {
        /* there is data pending to be sent, monitor the socket
                in the write set so we know when it is ready to take more
                data */
        FD_SET(current->socket, &write_set);
      }

      if (current->socket > max)
      {
        /* update max if necessary */
        max = current->socket;
      }
    }

    time_out.tv_usec = 100000; /* 1-tenth of a second timeout */
    time_out.tv_sec = 0;

    /* invoke select, make sure to pass max+1 !!! */
    /* Select() function is destructive on the sets */
    select_retval = select(max + 1, &read_set, &write_set, NULL, &time_out);
    if (select_retval < 0)
    {
      perror("select failed");
      abort();
    }

    if (select_retval == 0)
    {
      /* no descriptor ready, timeout happened */
      continue;
    }

    if (select_retval > 0) /* at least one file descriptor is ready */
    {
      if (FD_ISSET(sock, &read_set)) /* check the server socket */
      {
        /* there is an incoming connection, try to accept it */
        new_sock = accept(sock, (struct sockaddr *)&addr, &addr_len);

        if (new_sock < 0)
        {
          perror("error accepting connection");
          abort();
        }

        /* make the socket non-blocking so send and recv will
                 return immediately if the socket is not ready.
                 this is important to ensure the server does not get
                 stuck when trying to send data to a socket that
                 has too much data to send already.
               */
        if (fcntl(new_sock, F_SETFL, O_NONBLOCK) < 0)
        {
          perror("making socket non-blocking");
          abort();
        }

        /* the connection is made, everything is ready */
        /* let's see who's connecting to us */
        printf("Accepted connection. Client IP address is: %s\n",
               inet_ntoa(addr.sin_addr));

        /* remember this client connection in our linked list */
        add(&head, new_sock, addr);

        /* let's send a message to the client just for fun */
        // count = send(new_sock, message, strlen(message) + 1, 0);
        // if (count < 0)
        // {
        //   perror("error sending message to client");
        //   abort();
        // }
      }

      /* check other connected sockets, see if there is
               anything to read or some socket is ready to send
               more pending data */
      for (current = head.next; current; current = next)
      {
        next = current->next;

        /* see if we can now do some previously unsuccessful writes */
        if (FD_ISSET(current->socket, &write_set))
        {
          /* the socket is now ready to take more data */
          /* the socket data structure should have information
                   describing what data is supposed to be sent next.
                 */
          count = send(current->socket,
                       current->sendBuf,
                       current->sendLen,
                       MSG_DONTWAIT);

          if (count < 0)
          {
            if (errno == EAGAIN)
            {
              /* we are trying to dump too much data down the socket,
                 it cannot take more for the time being
                 will have to go back to select and wait til select
                 tells us the socket is ready for writing
              */
              /* nothing need to be done*/
            }
            else
            {
              /* something else is wrong */
              printf("send() error");
            }
          }
          /* note that it is important to check count for exactly
                   how many bytes were actually sent even when there are
                   no error. send() may send only a portion of the buffer
                   to be sent.
          */
          if (current->sendLen - count > 0)
          {
            current->sendBuf += count;
            current->sendLen -= count;
          }
          else
          {
            current->pending_data = 0;
          }
        }

        if (FD_ISSET(current->socket, &read_set))
        {
          /* we have data from a client */
          printf("recving...\n");
          count = recv(current->socket, buf, BUF_LEN, 0);

          if (count <= 0)
          {
            /* something is wrong */
            if (count == 0)
            {
              printf("Client closed connection. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
            }
            else
            {
              perror("error receiving from a client");
            }

            /* connection is closed, clean up */
            close(current->socket);
            dump(&head, current->socket);
          }
          else
          {
            /* we got count bytes of data from the client */
            /* in general, the amount of data received in a recv()
               call may not be a complete application message. it
               is important to check the data received against
               the message format you expect. if only a part of a
               message has been received, you must wait and
               receive the rest later when more data is available
               to be read */
            /* in this case, we expect a message where the first byte
                           stores the number of bytes used to encode a number,
                           followed by that many bytes holding a numeric value */

            /* append data */
            //printf("Fore time recvBuf specified %d\n", ntohs(*(short *)current->receBuf));
            // *(current->receBuf + current->receLen) = *buf;
            memcpy(current->receBuf + current->receLen, buf, count);
            
            current->receLen += count;
            //printf("Buff time specified %d\n", ntohs(*(short *)buf));
           // printf("First Count =  %d\n", current->receLen);

            /* check if everyting is received */
            if (current->expectedRece == -1)
            {
              //printf("Not specified Length\n");
              /* if expected receiving length not specified*/
              if (current->receLen >= 2)
              {
                current->expectedRece = ntohs(*(short *)current->receBuf);
                //printf("First time specified %d\n", current->expectedRece);
                //printf("First time Recev Lenght %d\n", current->receLen);
              }
            }

            if (current->receLen < current->expectedRece || current->expectedRece == -1)
            {
              /* do nothing, insert debug message*/
              current->pending_rece = 1;
            }
            else
            {
              //printf("Current Recev Lenght %d\n", current->receLen);
              //printf("Current Expected Lenght %d\n", current->expectedRece);
              /* everything received*/
              current->pending_rece = 0;
              /* process timestamp*/
              printf("Full message received!\n");
              printf("parsing msg...\n");

              long long *timestampPtr = (long long *)((short *)current->receBuf + 1);
              long long tv_sec = ntohll(*timestampPtr);
              long long tv_usec = ntohll(*(timestampPtr + 1));

              printf("Client sent at time %lld:%lld. Client IP address is: %s\n",
                     tv_sec, tv_usec, inet_ntoa(current->client_addr.sin_addr));
            }

            /* send return message*/
            /* dont need to consider if the socket need to receive while sending*/
            if (!current->pending_rece)
            {
              current->pending_data = 1;
              current->sendBuf = current->receBuf;
              current->sendLen = current->receLen;

              /* clear recv condition*/
              current->receLen = 0;
              current->expectedRece = -1;
            }
          }
        }
      }
    }
  }
}
