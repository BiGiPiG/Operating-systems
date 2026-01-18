#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <string.h>

#define ARRAY_SIZE 20

char shared_buffer[ARRAY_SIZE] = {0};
sem_t sync_sem;
atomic_int writer_done = 0;

void* writer_thread(void *args) {
    (void) args;
    int counter = 1;
    while (counter <= 10) {
        sem_wait(&sync_sem);
        snprintf(shared_buffer, ARRAY_SIZE, "%d", counter);
        sem_post(&sync_sem);
        counter++;
        sleep(1);
    }
    atomic_store(&writer_done, 1);
    return NULL;
}

void* reader_thread(void *args) {
    (void) args;
    while (atomic_load(&writer_done) == 0) {
        sem_wait(&sync_sem);
        printf("[Reader] Array content: \"%s\"\n", shared_buffer);
        sem_post(&sync_sem);
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t writer, reader;

    if (sem_init(&sync_sem, 0, 1) == -1) {
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

    sem_destroy(&sync_sem);
    return 0;
}