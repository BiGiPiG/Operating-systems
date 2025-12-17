#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#define SHM_SIZE 256

static char *shared_mem = NULL;
static int semid = -1;

int sem_lock(int semid) {
    struct sembuf sb = {0, -1, SEM_UNDO};
    return semop(semid, &sb, 1);
}

int sem_unlock(int semid) {
    struct sembuf sb = {0, 1, SEM_UNDO};
    return semop(semid, &sb, 1);
}

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

    key_t shm_key = ftok("shm.key", 'S');
    key_t sem_key = ftok("shm.key", 'X');
    if (shm_key == -1 || sem_key == -1) {
        perror("ftok — убедитесь, что файл shm.key существует (запустите sender)");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(shm_key, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget (receiver)");
        exit(EXIT_FAILURE);
    }

    semid = semget(sem_key, 1, 0666);
    if (semid == -1) {
        perror("semget (receiver)");
        exit(EXIT_FAILURE);
    }

    shared_mem = (char *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    printf("Receiver (PID: %d) подключился.\n", getpid());

    while (1) {
        if (sem_lock(semid) == -1) {
            perror("sem_lock (receiver)");
            break;
        }

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        if (!tm_info) {
            sem_unlock(semid);
            sleep(1);
            continue;
        }

        char local_copy[SHM_SIZE];
        strncpy(local_copy, shared_mem, SHM_SIZE - 1);
        local_copy[SHM_SIZE - 1] = '\0';

        if (sem_unlock(semid) == -1) {
            perror("sem_unlock (receiver)");
        }

        printf("Receiver Time: %02d:%02d:%02d | PID: %d | Data: %s\n",
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
               getpid(), local_copy);

        sleep(1);
    }

    receiver_cleanup(0);
    return 0;
}