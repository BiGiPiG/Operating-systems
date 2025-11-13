#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

int main() {
    char *fifo = "myfifo";
    char buffer[256];
    time_t rawtime;
    struct tm *timeinfo;

    if (mkfifo(fifo, 0666) && errno != EEXIST) {
        perror("Fifo creation error");
        exit(1);
    }

    printf("Waiting for readers...\n");
    sleep(5);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    snprintf(buffer, sizeof(buffer), "Time: %sPID: %d", asctime(timeinfo), getpid());

    int fd = open(fifo, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        printf("No readers connected after 5 seconds\n");
        unlink(fifo);
        exit(1);
    }

    write(fd, buffer, strlen(buffer) + 1);
    close(fd);
    
    unlink(fifo);
    printf("Message sent successfully\n");

    return 0;
}