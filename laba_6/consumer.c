#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

int main() {
    char *fifo = "myfifo";
    char buffer[256];
    time_t rawtime;
    struct tm *timeinfo;

    int fd = open(fifo, O_RDONLY);
    if (fd == -1) {
        perror("Error opening FIFO");
        fprintf(stderr, "Make sure another process is writing to the FIFO\n");
        exit(EXIT_FAILURE);
    }

    printf("Reading from FIFO...\n");
    read(fd, buffer, sizeof(buffer));
    close(fd);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    printf("Current time: %s", asctime(timeinfo));
    printf("Received: %s\n", buffer);

    unlink(fifo);

    return 0;
}
