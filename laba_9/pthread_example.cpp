#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define ARRAY_SIZE 20

char shared_buffer[ARRAY_SIZE] = {0};
sem_t mutex;
volatile int writer_done = 0;

void* writer_thread(void* arg) {
    int counter = 1;
    while (counter <= 10) {
        sem_wait(&mutex);
        snprintf(shared_buffer, ARRAY_SIZE, "%d", counter);
        sem_post(&mutex);
        counter++;
        sleep(1);
    }
    writer_done = 1;
    return NULL;
}

void* reader_thread(void* arg) {
    pid_t tid = syscall(SYS_gettid);
    while (!writer_done) {
        sem_wait(&mutex);
        printf("[Reader TID: %d] Array content: \"%s\"\n", (int)tid, shared_buffer);
        sem_post(&mutex);
        usleep(200000);
    }
    return NULL;
}

int main() {
    pthread_t writer, reader;

    if (sem_init(&mutex, 0, 1) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
        perror("pthread_create writer");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&reader, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create reader");
        exit(EXIT_FAILURE);
    }

    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    sem_destroy(&mutex);

    return 0;
}