#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int find_substring(const char *str, const char *pattern) {
    const char *result = strstr(str, pattern);
    return (result != NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Ошибка: не указан pattern\n");
        return 1;
    }
    
    char *pattern = argv[1];
    FILE *file = NULL;
    int use_stdin = (argc == 2);
    char *filename = NULL;
    
    if (use_stdin) {
        file = stdin;
    } else {
        filename = argv[2];
        file = fopen(filename, "r");
        if (file == NULL) {
            perror(filename);
            return 1;
        }
    }

    char buffer[BUFFER_SIZE];
    int matches_found = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        if (find_substring(buffer, pattern)) {
            matches_found = 1;
            printf("%s\n", buffer);
        }
    }

    if (!use_stdin) {
        fclose(file);
    }

    return (matches_found ? 0 : 1);
}