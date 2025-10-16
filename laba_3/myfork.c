#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

void atexit_handler(void) {
    printf("[atexit] Процесс %d завершается.\n", (int)getpid());
}

void sigint_handler(int sig) {
    printf("Процесс %d получил сигнал SIGINT (%d)\n", (int)getpid(), sig);
    exit(0);
}

void sigterm_handler(int sig, siginfo_t *info, void *context) {
    printf("Процесс %d получил сигнал SIGTERM (%d)\n", (int)getpid(), sig);
    exit(0);
}

int main(void) {
    pid_t pid;

    if (atexit(atexit_handler) != 0) {
        perror("atexit");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal SIGINT");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigterm_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }

    printf("Родительский процесс: PID=%d, PPID=%d\n", (int)getpid(), (int)getppid());

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("Дочерний процесс: PID=%d, PPID=%d\n", (int)getpid(), (int)getppid());
        sleep(2);
        exit(42);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Родитель: дочерний процесс %d завершился с кодом %d\n",
                   (int)pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Родитель: дочерний процесс %d завершился из-за сигнала %d\n",
                   (int)pid, WTERMSIG(status));
        }
    }

    return 0;
}