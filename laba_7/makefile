CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS =

TARGETS = sender receiver
KEYFILE = shm.key

.PHONY: all clean setup run-sender run-receiver

all: $(TARGETS)

sender: sender.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

receiver: receiver.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

setup:
	touch $(KEYFILE)

clean:
	rm -f $(TARGETS) $(KEYFILE)

run-sender: sender
	./sender

run-receiver: receiver
	./receiver