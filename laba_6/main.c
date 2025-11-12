#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[256];
    time_t rawtime;
    struct tm *timeinfo;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        close(pipefd[1]);

        ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            time(&rawtime);
            timeinfo = localtime(&rawtime);
            printf("Дочерний процесс (PID: %d): Текущее время: %s", getpid(), asctime(timeinfo));
            printf("Полученная строка: %s", buffer);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[0]);

        sleep(5);

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        snprintf(buffer, sizeof(buffer), "Текущее время: %sPID процесса: %d\n", asctime(timeinfo), getpid());

        write(pipefd[1], buffer, strlen(buffer));
        close(pipefd[1]);

        wait(NULL);
    }

    return 0;
}