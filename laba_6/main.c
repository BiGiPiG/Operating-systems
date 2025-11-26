#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
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
    int pipefd[2];
    pid_t pid;
    char buffer[256];
    char time_buffer[64];

    if (pipe(pipefd) == -1) {
        perror("Pipe creation error");
        exit(1);
    }

    pid = fork();
    if (pid == -1) {
        perror("Fork creation error");
        close(pipefd[0]);
        close(pipefd[1]);
        exit(1);
    }

    if (pid == 0) { 
        close(pipefd[1]);

        get_current_time_with_ms(time_buffer, sizeof(time_buffer));
        printf("Child (PID: %d) started waiting at: %s\n", getpid(), time_buffer);

        ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            get_current_time_with_ms(time_buffer, sizeof(time_buffer));
            
            printf("\n════════ CHILD PROCESS ════════\n");
            printf("PID: %d\n", getpid());
            printf("Data RECEIVED at: %s\n", time_buffer);
            printf("Message: %s\n", buffer);
            printf("═══════════════════════════════\n\n");
        }

        close(pipefd[0]);
        
        get_current_time_with_ms(time_buffer, sizeof(time_buffer));
        printf("Child completed at: %s\n", time_buffer);
    } else {
        close(pipefd[0]);

        get_current_time_with_ms(time_buffer, sizeof(time_buffer));
        printf("Parent (PID: %d) started at: %s\n", getpid(), time_buffer);

        get_current_time_with_ms(time_buffer, sizeof(time_buffer));
        
        snprintf(buffer, sizeof(buffer), 
                "Parent PID: %d | Data SENT at: %s", 
                getpid(), time_buffer);

        printf("Parent sending at: %s\n", time_buffer);

        printf("Waiting 5 seconds...\n");
        sleep(5);
        
        write(pipefd[1], buffer, strlen(buffer) + 1);
        close(pipefd[1]);

        wait(NULL);
        
        get_current_time_with_ms(time_buffer, sizeof(time_buffer));
        printf("Parent completed at: %s\n", time_buffer);
    }

    return 0;
}