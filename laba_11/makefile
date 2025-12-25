# Имя исполняемого файла
TARGET = reader_writer_condvar

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -pthread
# -pthread автоматически добавляет нужные флаги для компиляции и линковки с pthread

# Исходные файлы (у вас только один)
SRCS = main.c

# Объектные файлы
OBJS = $(SRCS:.c=.o)

# Сборка программы
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Компиляция .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(OBJS) $(TARGET)

# Запуск (опционально)
run: $(TARGET)
	./$(TARGET)

.PHONY: clean run