#include <stdlib.h>
#include <stdio.h>

int main() {
    FILE* fp;
    fp = fopen("filetest.txt", "r");
    char* buffer = (char*)malloc(2);
    int t = fread(buffer, 1, 2, fp);
    printf("%s\n", buffer);
    printf("%d char read\n", t);

    t = fread(buffer, 1, 2, fp);
    printf("%s\n", buffer);
    printf("%d char read\n", t);

    t = fread(buffer, 1, 2, fp);
    printf("%s\n", buffer);
    printf("%d char read\n", t);
}