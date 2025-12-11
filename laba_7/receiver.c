#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <string.h>
#include <signal.h>


#define SHM_SIZE 256

static char *shared_mem = NULL;

void receiver_cleanup(int sig) {
    (void)sig;
    if (shared_mem != NULL && shared_mem != (void *)-1) {
        shmdt(shared_mem);
    }
    exit(EXIT_SUCCESS);
}

int main() {
    signal(SIGINT, receiver_cleanup);
    signal(SIGTERM, receiver_cleanup);

    key_t key = ftok("shm.key", 'S');
    if (key == -1) {
        perror("ftok — убедитесь, что sender запущен и shm.key существует");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget (receiver)");
        exit(EXIT_FAILURE);
    }

    shared_mem = (char *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    printf("Receiver (PID: %d) подключился.\n", getpid());

    while (1) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        if (!tm_info) continue;

        char local_copy[SHM_SIZE];
        strncpy(local_copy, shared_mem, SHM_SIZE - 1);
        local_copy[SHM_SIZE - 1] = '\0';

        printf("Receiver Time: %02d:%02d:%02d | PID: %d | Data: %s\n",
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
               getpid(), local_copy);
        sleep(1);
    }

    receiver_cleanup(0);
    return 0;
}

