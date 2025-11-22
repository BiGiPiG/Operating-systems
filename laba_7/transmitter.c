#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"
#define LOCKFILE "/tmp/sender.lock"
#define BUF_SIZE 256

int main() {
    int lock_fd = open(LOCKFILE, O_CREAT | O_RDWR, 0666);
    if (lock_fd == -1) {
        perror("open lock file");
        exit(EXIT_FAILURE);
    }
    if (lockf(lock_fd, F_TLOCK, 0) == -1) {
        printf("Передающая программа уже запущена.\n");
        close(lock_fd);
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, BUF_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void* ptr = mmap(0, BUF_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem);
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char buffer[BUF_SIZE];
        snprintf(buffer, BUF_SIZE, "Time: %02d:%02d:%02d PID: %d",
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, getpid());
        strncpy((char*)ptr, buffer, BUF_SIZE);
        sem_post(sem);
        sleep(1);
    }

    munmap(ptr, BUF_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    close(lock_fd);
    unlink(LOCKFILE);

    return 0;
}
