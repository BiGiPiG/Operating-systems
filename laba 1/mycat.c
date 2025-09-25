#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define BUFFER_SIZE 4096

void process_file(FILE *file, int number_all, int number_nonblank, int show_ends) {
    char buffer[BUFFER_SIZE];
    int line_number = 1;
    int empty_line = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        size_t len = strlen(buffer);
        
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            empty_line = (len == 1);
        } else {
            empty_line = 0;
        }

        if (number_all) {
            printf("%6d\t", line_number++);
        } else if (number_nonblank) {
            if (!empty_line && buffer[0] != '\0') {
                printf("%6d\t", line_number++);
            } else {
                printf("      \t");
            }
        }

        printf("%s", buffer);

        if (show_ends && len > 0 && buffer[len - 1] != '\n') {
            printf("$");
        }

        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    int number_all = 0;
    int number_nonblank = 0;
    int show_ends = 0;
    int opt;
    
    struct option long_options[] = {
        {"number", no_argument, 0, 'n'},
        {"number-nonblank", no_argument, 0, 'b'},
        {"show-ends", no_argument, 0, 'E'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "nbEh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'n':
                number_all = 1;
                break;
            case 'b':
                number_nonblank = 1;
                break;
            case 'E':
                show_ends = 1;
                break;
        }
    }

    if (number_all && number_nonblank) {
        fprintf(stderr, "Ошибка: флаги -n и -b не могут использоваться вместе\n");
        return 1;
    }

    if (optind >= argc) {
        process_file(stdin, number_all, number_nonblank, show_ends);
    } else {
        for (int i = optind; i < argc; i++) {
            FILE *file = fopen(argv[i], "r");
            if (file == NULL) {
                perror(argv[i]);
                continue;
            }
            
            process_file(file, number_all, number_nonblank, show_ends);
            fclose(file);
        }
    }

    return 0;
}
