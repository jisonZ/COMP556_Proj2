#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>


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
    int isFileCreated = 0;
    char * subdir = ".";
    char * fileName = "test.txt";
    struct stat st;
    memset(&st, 0, sizeof(st));
    if (stat(subdir, &st) == -1) {
      char *dir;
      printf("-- Creating new directory %s...\n", subdir);
      dir = strdup(subdir);
      mkdir(dir, 0777);
      free(dir);
  }

  if (!isFileCreated) {
      printf("-- Creating file %s...\n", fileName);
      char * target_file_name;
      target_file_name = strdup(subdir);
      strcat(target_file_name, "/");
      strcat(target_file_name, fileName);
      fp = fopen(target_file_name, "w");
      isFileCreated = 1;
      fwrite(target_file_name, 1, strlen(fileName) + 2, fp);
      free(target_file_name);
  }

    fclose(fp);
    return 0;
}