// sender.c
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

static int shmid = -1;
static int semid = -1;
static char *shared_mem = NULL;

int sem_lock(int semid) {
    struct sembuf sb = {0, -1, SEM_UNDO};
    return semop(semid, &sb, 1);
}

int sem_unlock(int semid) {
    struct sembuf sb = {0, 1, SEM_UNDO};
    return semop(semid, &sb, 1);
}

void cleanup(int sig) {
    (void)sig;

    if (shared_mem != NULL && shared_mem != (void *)-1) {
        shmdt(shared_mem);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }
    unlink("shm.key");

    printf("\nSender завершён корректно.\n");
    exit(EXIT_SUCCESS);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    FILE *fp = fopen("shm.key", "w");
    if (fp) {
        fclose(fp);
    } else {
        perror("Не удалось создать shm.key");
        exit(EXIT_FAILURE);
    }

    key_t shm_key = ftok("shm.key", 'S');
    key_t sem_key = ftok("shm.key", 'X');
    if (shm_key == -1 || sem_key == -1) {
        perror("ftok");
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    shmid = shmget(shm_key, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Ошибка: сегмент разделяемой памяти уже существует. Убедитесь, что sender не запущен.\n");
        } else {
            perror("shmget");
        }
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    semid = semget(sem_key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Ошибка: семафор уже существует.\n");
        } else {
            perror("semget");
        }
        shmctl(shmid, IPC_RMID, NULL);
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl SETVAL");
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, IPC_RMID, 0);
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    shared_mem = (char *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, IPC_RMID, 0);
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    printf("Sender (PID: %d) запущен. Отправка данных каждые 3 секунды...\n", getpid());

    while (1) {
        if (sem_lock(semid) == -1) {
            perror("sem_lock (sender)");
            break;
        }

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(shared_mem, SHM_SIZE,
                     "Time: %02d:%02d:%02d | Sender PID: %d",
                     tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, getpid());
        }

        if (sem_unlock(semid) == -1) {
            perror("sem_unlock (sender)");
        }

        sleep(3);
    }

    cleanup(0);
    return 0;
}