#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <mode> <file1> [file2 ...]\n", program_name);
    fprintf(stderr, "Modes can be:\n");
    fprintf(stderr, "  Symbolic: [ugoa]*[+-=][rwx]+\n");
    fprintf(stderr, "  Octal: [0-7][0-7][0-7]\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s +x file.txt\n", program_name);
    fprintf(stderr, "  %s u-r file.txt\n", program_name);
    fprintf(stderr, "  %s 755 file.txt\n", program_name);
}

// Функция для обработки символьного режима
int apply_symbolic_mode(const char *mode_str, mode_t current_mode, mode_t *new_mode) {
    *new_mode = current_mode;
    
    const char *p = mode_str;
    
    // Парсим целевую группу пользователей [ugoa]*
    int users = 0; // битовая маска: u=1, g=2, o=4, a=7
    while (*p && *p != '+' && *p != '-' && *p != '=') {
        switch (*p) {
            case 'u': users |= 1; break;
            case 'g': users |= 2; break;
            case 'o': users |= 4; break;
            case 'a': users |= 7; break;
            default:
                fprintf(stderr, "Invalid user specification: %c\n", *p);
                return -1;
        }
        p++;
    }
    
    // Если группа не указана, используем 'a'
    if (users == 0) users = 7;
    
    // Парсим оператор [+-=]
    if (*p != '+' && *p != '-' && *p != '=') {
        fprintf(stderr, "Expected operator (+, -, =), got: %c\n", *p);
        return -1;
    }
    char operation = *p;
    p++;
    
    // Парсим права доступа [rwx]*
    int permissions = 0;
    while (*p) {
        switch (*p) {
            case 'r': permissions |= 4; break;
            case 'w': permissions |= 2; break;
            case 'x': permissions |= 1; break;
            default:
                fprintf(stderr, "Invalid permission: %c\n", *p);
                return -1;
        }
        p++;
    }
    
    // Применяем операцию к каждой указанной группе
    if (users & 1) { // user (владелец)
        if (operation == '+') {
            *new_mode |= (permissions << 6);
        } else if (operation == '-') {
            *new_mode &= ~(permissions << 6);
        } else if (operation == '=') {
            *new_mode &= ~(7 << 6); // очищаем биты владельца
            *new_mode |= (permissions << 6);
        }
    }
    
    if (users & 2) { // group (группа)
        if (operation == '+') {
            *new_mode |= (permissions << 3);
        } else if (operation == '-') {
            *new_mode &= ~(permissions << 3);
        } else if (operation == '=') {
            *new_mode &= ~(7 << 3); // очищаем биты группы
            *new_mode |= (permissions << 3);
        }
    }
    
    if (users & 4) { // others (остальные)
        if (operation == '+') {
            *new_mode |= permissions;
        } else if (operation == '-') {
            *new_mode &= ~permissions;
        } else if (operation == '=') {
            *new_mode &= ~7; // очищаем биты остальных
            *new_mode |= permissions;
        }
    }
    
    return 0;
}

// Функция для обработки octal режима
int apply_octal_mode(const char *mode_str, mode_t *new_mode) {
    // Проверяем, что строка состоит из 3 цифр
    if (strlen(mode_str) != 3) {
        fprintf(stderr, "Octal mode must be 3 digits\n");
        return -1;
    }
    
    for (int i = 0; i < 3; i++) {
        if (mode_str[i] < '0' || mode_str[i] > '7') {
            fprintf(stderr, "Invalid octal digit: %c\n", mode_str[i]);
            return -1;
        }
    }
    
    *new_mode = 0;
    *new_mode |= (mode_str[0] - '0') << 6; // user
    *new_mode |= (mode_str[1] - '0') << 3; // group
    *new_mode |= (mode_str[2] - '0');      // others
    
    return 0;
}

// Функция для определения типа режима (symbolic или octal)
int parse_mode(const char *mode_str, mode_t current_mode, mode_t *new_mode) {
    // Проверяем, является ли режим octal (только цифры)
    int is_octal = 1;
    for (const char *p = mode_str; *p; p++) {
        if (*p < '0' || *p > '7') {
            is_octal = 0;
            break;
        }
    }
    
    if (is_octal && strlen(mode_str) == 3) {
        return apply_octal_mode(mode_str, new_mode);
    } else {
        return apply_symbolic_mode(mode_str, current_mode, new_mode);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *mode_str = argv[1];
    int success_count = 0;
    
    // Обрабатываем каждый файл
    for (int i = 2; i < argc; i++) {
        const char *filename = argv[i];
        
        // Получаем текущие права доступа файла
        struct stat st;
        if (stat(filename, &st) == -1) {
            perror(filename);
            continue;
        }
        
        mode_t current_mode = st.st_mode;
        mode_t new_mode;
        
        // Парсим и применяем новый режим
        if (parse_mode(mode_str, current_mode, &new_mode) == -1) {
            fprintf(stderr, "Invalid mode: %s\n", mode_str);
            continue;
        }
        
        // Применяем новые права доступа
        if (chmod(filename, new_mode) == -1) {
            perror(filename);
            continue;
        }
        
        printf("Changed mode of '%s' from %04o to %04o\n", 
               filename, current_mode & 0777, new_mode & 0777);
        success_count++;
    }
    
    if (success_count == 0) {
        fprintf(stderr, "No files were successfully processed\n");
        return 1;
    }
    
    return 0;
}