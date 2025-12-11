#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define ARRAY_SIZE 100
char shared_array[ARRAY_SIZE];
pthread_mutex_t mutex;
int write_counter = 0;

void* writer_thread() {
    while (1) {
        pthread_mutex_lock(&mutex);

        write_counter++;

        snprintf(shared_array, ARRAY_SIZE, "Запись %d", write_counter);

        printf("Пишущий поток: записал '%s'\n", shared_array);

        pthread_mutex_unlock(&mutex);

        sleep(2);
    }
    return NULL;
}

void* reader_thread(void* arg) {
    long tid = (long)arg;

    while (1) {
        pthread_mutex_lock(&mutex);

        printf("Читающий поток %ld: прочитал '%s'\n", tid, shared_array);

        pthread_mutex_unlock(&mutex);

        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t writer_tid;
    pthread_t reader_tids[10];
    int i;

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Ошибка инициализации мьютекса");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&writer_tid, NULL, writer_thread, NULL) != 0) {
        perror("Ошибка создания пишущего потока");
        pthread_mutex_destroy(&mutex);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 10; i++) {
        if (pthread_create(&reader_tids[i], NULL, reader_thread, (void*)(long)(i+1)) != 0) {
            perror("Ошибка создания читающего потока");
            for (int j = 0; j < i; j++) {
                pthread_cancel(reader_tids[j]);
            }
            pthread_cancel(writer_tid);
            pthread_mutex_destroy(&mutex);
            exit(EXIT_FAILURE);
        }
    }

    sleep(10);

    pthread_cancel(writer_tid);
    for (i = 0; i < 10; i++) {
        pthread_cancel(reader_tids[i]);
    }

    pthread_join(writer_tid, NULL);
    for (i = 0; i < 10; i++) {
        pthread_join(reader_tids[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    printf("Программа завершена.\n");
    return 0;
}