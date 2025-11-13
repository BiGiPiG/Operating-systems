#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
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
    char send_time[64];

    if (mkfifo(fifo, 0666) && errno != EEXIST) {
        perror("Fifo creation error");
        exit(1);
    }

    printf("Waiting for readers...\n");
    sleep(5);

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    get_current_time_with_ms(send_time, sizeof(send_time));
    snprintf(buffer, sizeof(buffer), 
             "Producer PID: %d | Data sent: %s", 
             getpid(), send_time);

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