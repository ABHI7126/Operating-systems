#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Missing time argument\n");
        return 1;
    }

    int seconds = atoi(argv[1]);
    printf("Sleeping for %d seconds\n", seconds);
    sleep(seconds);
    return 0;
}
