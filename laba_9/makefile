CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -D_XOPEN_SOURCE=700
LDFLAGS = -pthread

# Цели
TARGETS = pthread_example sender receiver

# Правила по умолчанию
all: $(TARGETS)

# Компиляция программы с pthread и semaphores (внутрипроцессные)
pthread_example: pthread_example.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# Sender (System V shared memory + semaphore)
sender: sender.c
	$(CC) $(CFLAGS) -o $@ $<

# Receiver (System V shared memory + semaphore)
receiver: receiver.c
	$(CC) $(CFLAGS) -o $@ $<

# Очистка
clean:
	rm -f $(TARGETS) shm.key

.PHONY: all clean distclean