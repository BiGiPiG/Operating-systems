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

void remove_old_ipc(key_t shm_key, key_t sem_key) {
    int old_shmid = shmget(shm_key, 0, 0);
    if (old_shmid != -1) {
        shmctl(old_shmid, IPC_RMID, NULL);
        printf("Удалён старый сегмент разделяемой памяти.\n");
    }

    int old_semid = semget(sem_key, 0, 0);
    if (old_semid != -1) {
        semctl(old_semid, 0, IPC_RMID);
        printf("Удалён старый семафор.\n");
    }
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    FILE *fp = fopen("shm.key", "w");
    if (!fp) {
        perror("Не удалось создать shm.key");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%d", getpid());
    fclose(fp);

    key_t shm_key = ftok("shm.key", 'S');
    key_t sem_key = ftok("shm.key", 'X');
    if (shm_key == -1 || sem_key == -1) {
        perror("ftok");
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    remove_old_ipc(shm_key, sem_key);

    shmid = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget (sender)");
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    semid = semget(sem_key, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget (sender)");
        shmctl(shmid, IPC_RMID, NULL);
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } sem_arg;
    sem_arg.val = 1;
    if (semctl(semid, 0, SETVAL, sem_arg) == -1) {
        perror("semctl SETVAL");
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    shared_mem = (char *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    strcpy(shared_mem, "Initial data");

    printf("Sender (PID: %d) запущен. Отправка данных каждые 3 секунды...\n", getpid());

    while (1) {
        if (sem_lock(semid) == -1) {
            if (errno == EINTR) continue;
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
            if (errno != EINTR) perror("sem_unlock (sender)");
        }

        sleep(3);
    }

    cleanup(0);
    return 0;
}