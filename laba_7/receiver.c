#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"
#define BUF_SIZE 256

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void* ptr = mmap(0, BUF_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem);
        char buffer[BUF_SIZE];
        strncpy(buffer, (char*)ptr, BUF_SIZE);
        sem_post(sem);

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        printf("Receiver Time: %02d:%02d:%02d PID: %d | Sender Data: %s\n",
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, getpid(), buffer);

        sleep(1);
    }

    munmap(ptr, BUF_SIZE);
    close(shm_fd);
    sem_close(sem);

    return 0;
}
