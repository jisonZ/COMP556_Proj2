#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) 
{
    char* fname = "test.txt";
    int fsize;

    if (argc != 2) {
        perror("Usage: ./gen_file <size in MB>");
        abort();
    } else {
        // fname = argv[1];
        fsize = atoi(argv[1]);
    }

    FILE* fp;
    fp = fopen(fname, "w");
    for (int i = 0; i < fsize*1e6; ++i) {
        char randomChar = 'A' + (random() % 26);
        fprintf(fp, "%c", randomChar);
    }
}