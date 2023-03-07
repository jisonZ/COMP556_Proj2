#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char *recv_host = NULL;
    char *recv_port = NULL;
    char *subdir = NULL;
    char *filename = NULL;
    char whole_filename[100];

    int opt;
    while ((opt = getopt(argc, argv, "r:f:")) != -1) {
        switch (opt) {
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
    if (recv_host == NULL || recv_port == NULL || subdir == NULL || filename == NULL) {
        printf("Usage: sendfile -r <recv host>:<recv port> -f <subdir>/<filename>\n");
        return 1;
    }

    // Print out the parsed arguments
    printf("Receiver host: %s\nReceiver port: %s\nSubdirectory: %s\nFilename: %s\n%s", recv_host, recv_port, subdir, filename, whole_filename);

    return 0;
}