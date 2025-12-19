# Имя исполняемого файла
TARGET = reader_writer

# Компилятор и флаги
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2 -pthread

# Исходный файл
SRC = main.c

# Объектный файл
OBJ = $(SRC:.c=.o)

# Сборка
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $<

# Компиляция .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(OBJ) $(TARGET)