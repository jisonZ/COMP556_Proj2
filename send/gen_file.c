#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) 
{
    char* fname = "test.txt";
    int fsize;
    int i;
    int newLine = 1;

    if (argc != 2) {
        perror("Usage: ./gen_file <size in MB>");
        abort();
    } else {
        // fname = argv[1];
        fsize = atoi(argv[1]);
    }

    FILE* fp;
    fp = fopen(fname, "w");
    for (i = 0; i < fsize; ++i) {
        char randomChar = 'A' + (random() % 26);
        fprintf(fp, "%c", randomChar);
        if (newLine % 50 == 0) {
            fprintf(fp, "%c", '\n');
        }
        newLine++;
    }

    return 0;
}