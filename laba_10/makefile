TARGET = reader_writer
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2 -pthread
SRC = main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)