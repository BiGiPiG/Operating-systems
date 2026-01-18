#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define ARRAY_SIZE 100
#define NUM_READERS 10

char shared_array[ARRAY_SIZE] = "Начальное состояние";
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
volatile int write_counter = 0;
volatile int stop = 0;

void* writer_thread(void* arg) {
    (void)arg;
    while (!stop) {
        pthread_rwlock_wrlock(&rwlock);
        write_counter++;
        snprintf(shared_array, ARRAY_SIZE, "Запись %d", write_counter);
        printf("Писатель: записал '%s'\n", shared_array);
        pthread_rwlock_unlock(&rwlock);

        usleep(500000);
    }
    return NULL;
}

void* reader_thread(void* arg) {
    long tid = (long)arg;
    char local_copy[ARRAY_SIZE];

    while (!stop) {
        pthread_rwlock_rdlock(&rwlock);
        strncpy(local_copy, shared_array, ARRAY_SIZE - 1);
        local_copy[ARRAY_SIZE - 1] = '\0';
        pthread_rwlock_unlock(&rwlock);

        printf("Читатель %ld: прочитал '%s'\n", tid, local_copy);

        usleep(200000);
    }
    return NULL;
}

int main() {
    pthread_t writer_tid;
    pthread_t reader_tids[NUM_READERS];

    for (long i = 0; i < NUM_READERS; i++) {
        if (pthread_create(&reader_tids[i], NULL, reader_thread, (void*)(i + 1)) != 0) {
            perror("Ошибка создания читателя");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_create(&writer_tid, NULL, writer_thread, NULL) != 0) {
        perror("Ошибка создания писателя");
        exit(EXIT_FAILURE);
    }

    sleep(10);

    stop = 1;

    pthread_join(writer_tid, NULL);
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(reader_tids[i], NULL);
    }

    printf("Программа завершена.\n");
    return 0;
}