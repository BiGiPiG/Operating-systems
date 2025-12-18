CC = gcc

# Общие флаги
CFLAGS_COMMON = -Wall -Wextra -std=c99 -O2

# Флаги для pthread_example
CFLAGS_PTHREAD = $(CFLAGS_COMMON) -D_GNU_SOURCE -pthread
LDFLAGS_PTHREAD = -pthread

# Флаги для sender/receiver (System V IPC)
CFLAGS_IPC = $(CFLAGS_COMMON) -D_GNU_SOURCE

TARGETS = pthread_example sender receiver

all: $(TARGETS)

pthread_example: pthread_example.c
	$(CC) $(CFLAGS_PTHREAD) $(LDFLAGS_PTHREAD) -o $@ $<

sender: sender.c
	$(CC) $(CFLAGS_IPC) -o $@ $<

receiver: receiver.c
	$(CC) $(CFLAGS_IPC) -o $@ $<

clean:
	rm -f $(TARGETS) shm.key

.PHONY: all clean