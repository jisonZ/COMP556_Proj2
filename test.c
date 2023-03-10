#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>


int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int main() {
    // FILE* fp;
    // fp = fopen("filetest.txt", "r");
    // char* buffer = (char*)malloc(2);
    // int t = fread(buffer, 1, 2, fp);
    // printf("%s\n", buffer);
    // printf("%d char read\n", t);

    // t = fread(buffer, 1, 2, fp);
    // printf("%s\n", buffer);
    // printf("%d char read\n", t);

    // t = fread(buffer, 1, 2, fp);
    // printf("%s\n", buffer);
    // printf("%d char read\n", t);
    FILE* fp = fopen("test.txt", "r");
    char* buffer = (char*)malloc(8);
    size_t buffersize = 0;
    buffersize = fread(buffer, 1, 8, fp);
    printf("%s\n", buffer);
    printf("%i char read\n", buffersize);
    buffersize = fread(buffer, 1, 8, fp);
    printf("%s\n", buffer);
    printf("%i char read\n", buffersize);
    buffersize = fread(buffer, 1, 8, fp);
    printf("%s\n", buffer);
    printf("%i char read\n", buffersize);

    while(buffersize = fread(buffer, 1, 8, fp) != 0 ){
          printf("%s\n", buffer);
          printf("%i char read\n", buffersize);
          memset(buffer, 0, buffersize);
          // fseek(fp, buffersize, SEEK_CUR);
    }
   

    fclose(fp);
    return 0;
}