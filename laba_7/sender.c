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

static int shmid = -1;
static char *shared_mem = NULL;

void cleanup(int sig) {
    (void)sig;

    if (shared_mem != NULL && shared_mem != (void *)-1) {
        shmdt(shared_mem);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    unlink("shm.key");

    printf("\nSender завершён корректно.\n");
    exit(EXIT_SUCCESS);
}

int main() {
    signal(SIGINT, cleanup);   // Ctrl+C
    signal(SIGTERM, cleanup);  // kill

    FILE *fp = fopen("shm.key", "w");
    if (fp) {
        fclose(fp);
    } else {
        perror("Не удалось создать shm.key");
        exit(EXIT_FAILURE);
    }

    key_t key = ftok("shm.key", 'S');
    if (key == -1) {
        perror("ftok");
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    shmid = shmget(key, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Передающая программа уже запущена (сегмент уже существует).\n");
        } else {
            perror("shmget");
        }
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    shared_mem = (char *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        unlink("shm.key");
        exit(EXIT_FAILURE);
    }

    printf("Sender (PID: %d) запущен. Нажмите Ctrl+C для завершения.\n", getpid());

    while (1) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(shared_mem, SHM_SIZE,
                     "Time: %02d:%02d:%02d | Sender PID: %d",
                     tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, getpid());
        }
        sleep(1);
    }

    cleanup(0);
    return 0;
}