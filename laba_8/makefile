all: mutex

mutex: main.c
	gcc -Wall -Wextra -std=c99 -pthread -o mutex main.c

clean:
	rm -f mutex