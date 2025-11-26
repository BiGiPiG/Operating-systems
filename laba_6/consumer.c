#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

void get_current_time_with_ms(char *buffer, size_t size) {
    struct timeval tv;
    struct tm *timeinfo;
    
    gettimeofday(&tv, NULL);
    timeinfo = localtime(&tv.tv_sec);
    
    snprintf(buffer, size, "%02d:%02d:%02d.%03ld",
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
             tv.tv_usec / 1000);
}

int main() {
    char *fifo = "myfifo";
    char buffer[256];
    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[64];

    int fd = open(fifo, O_RDONLY);
    if (fd == -1) {
        perror("Error opening FIFO");
        exit(EXIT_FAILURE);
    }

    printf("Reading from FIFO...\n");
    read(fd, buffer, sizeof(buffer));
    close(fd);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    get_current_time_with_ms(time_buffer, sizeof(time_buffer));
    printf("Current time: %s (%s)\n", asctime(timeinfo), time_buffer);
    printf("Received: %s\n", buffer);

    unlink(fifo);

    return 0;
}
