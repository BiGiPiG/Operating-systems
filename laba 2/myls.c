#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define MAX_FILES 1024

typedef struct {
    char name[256];
    struct stat st;
    char link_target[1024];
} file_info;

void reset_color() { printf("\033[0m"); }
void blue_color() { printf("\033[1;34m"); }
void green_color() { printf("\033[1;32m"); } 
void cyan_color() { printf("\033[1;36m"); }   

void set_file_color(mode_t mode, int is_link) {
    if (is_link) {
        cyan_color();
    } else if (S_ISDIR(mode)) {
        blue_color();
    } else if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH) {
        green_color();
    }
}

void get_permissions(mode_t mode, char *str) {
    str[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' : '-';
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

int compare_files(const void *a, const void *b) {
    const file_info *fa = (const file_info *)a;
    const file_info *fb = (const file_info *)b;
    return strcmp(fa->name, fb->name);
}

void get_max_lengths(file_info *files, int count, int *max_links, int *max_user, 
                    int *max_group, int *max_size, int *max_name) {
    *max_links = *max_user = *max_group = *max_size = *max_name = 0;
    
    for (int i = 0; i < count; i++) {
        struct passwd *pw = getpwuid(files[i].st.st_uid);
        struct group *gr = getgrgid(files[i].st.st_gid);
        
        int links_len = snprintf(NULL, 0, "%ld", (long)files[i].st.st_nlink);
        int user_len = pw ? strlen(pw->pw_name) : 0;
        int group_len = gr ? strlen(gr->gr_name) : 0;
        int size_len = snprintf(NULL, 0, "%ld", (long)files[i].st.st_size);
        int name_len = strlen(files[i].name);
        
        if (links_len > *max_links) *max_links = links_len;
        if (user_len > *max_user) *max_user = user_len;
        if (group_len > *max_group) *max_group = group_len;
        if (size_len > *max_size) *max_size = size_len;
        if (name_len > *max_name) *max_name = name_len;
    }
}

void print_long_format(file_info *files, int count, int show_dot) {
    int total_blocks = 0;
    int max_links, max_user, max_group, max_size, max_name;

    int visible_count = 0;
    for (int i = 0; i < count; i++) {
        if (!show_dot && files[i].name[0] == '.') continue;
        total_blocks += files[i].st.st_blocks;
        visible_count++;
    }
    
    if (visible_count > 0) {
        printf("total %d\n", total_blocks / 2);
    }
    
    get_max_lengths(files, count, &max_links, &max_user, &max_group, &max_size, &max_name);
    
    for (int i = 0; i < count; i++) {
        if (!show_dot && files[i].name[0] == '.') continue;
        
        char permissions[11];
        get_permissions(files[i].st.st_mode, permissions);
        
        struct passwd *pw = getpwuid(files[i].st.st_uid);
        struct group *gr = getgrgid(files[i].st.st_gid);
        
        char time_str[64];
        struct tm *timeinfo = localtime(&files[i].st.st_mtime);
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", timeinfo);
        
        printf("%s %*ld %-*s %-*s %*ld %s ",
               permissions,
               max_links, (long)files[i].st.st_nlink,
               max_user, pw ? pw->pw_name : "unknown",
               max_group, gr ? gr->gr_name : "unknown",
               max_size, (long)files[i].st.st_size,
               time_str);
        
        int is_link = S_ISLNK(files[i].st.st_mode);
        set_file_color(files[i].st.st_mode, is_link);
        printf("%s", files[i].name);
        reset_color();
        
        if (is_link) {
            printf(" -> %s", files[i].link_target);
        }
        
        printf("\n");
    }
}

void print_normal_format(file_info *files, int count, int show_dot) {
    char *visible_files[count];
    int visible_count = 0;
    
    for (int i = 0; i < count; i++) {
        if (!show_dot && files[i].name[0] == '.') continue;
        visible_files[visible_count++] = files[i].name;
    }
    
    if (visible_count == 0) return;
    
    int max_name_len = 0;
    for (int i = 0; i < visible_count; i++) {
        int len = strlen(visible_files[i]);
        if (len > max_name_len) max_name_len = len;
    }
    
    int term_width = 80;
    int padding = 2;
    int col_width = max_name_len + padding;
    int cols = term_width / col_width;
    if (cols == 0) cols = 1;
    if (cols > visible_count) cols = visible_count;
    
    int rows = (visible_count + cols - 1) / cols;
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int idx = col * rows + row;
            if (idx < visible_count) {
                int file_idx = -1;
                for (int i = 0; i < count; i++) {
                    if (!show_dot && files[i].name[0] == '.') continue;
                    if (strcmp(files[i].name, visible_files[idx]) == 0) {
                        file_idx = i;
                        break;
                    }
                }
                
                if (file_idx != -1) {
                    char *name = files[file_idx].name;
                    int has_space = strchr(name, ' ') != NULL;
                    
                    set_file_color(files[file_idx].st.st_mode, 
                                 S_ISLNK(files[file_idx].st.st_mode));
                    
                    if (has_space) {
                        printf("'%s'", name);
                    } else {
                        printf("%s", name);
                    }
                    reset_color();
                    
                    int name_len = strlen(name) + (has_space ? 2 : 0);
                    if (col < cols - 1) {
                        int next_idx = (col + 1) * rows + row;
                        if (next_idx < visible_count) {
                            for (int j = name_len; j < col_width; j++) {
                                printf(" ");
                            }
                        }
                    }
                }
            }
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    int opt_l = 0, opt_a = 0;
    int opt;
    
    while ((opt = getopt(argc, argv, "la")) != -1) {
        switch (opt) {
            case 'l': opt_l = 1; break;
            case 'a': opt_a = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-a] [directory...]\n", argv[0]);
                exit(1);
        }
    }
    
    char *directories[argc];
    int dir_count = 0;
    
    for (int i = optind; i < argc; i++) {
        directories[dir_count++] = argv[i];
    }
    
    if (dir_count == 0) {
        directories[dir_count++] = ".";
    }
    
    for (int dir_idx = 0; dir_idx < dir_count; dir_idx++) {
        char *dir_name = directories[dir_idx];
        
        if (dir_count > 1) {
            printf("%s:\n", dir_name);
        }
        
        DIR *dir = opendir(dir_name);
        if (!dir) {
            perror(dir_name);
            continue;
        }
        
        file_info files[MAX_FILES];
        int file_count = 0;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
            if (!opt_a && (strcmp(entry->d_name, ".") == 0 || 
                          strcmp(entry->d_name, "..") == 0)) {
                continue;
            }
            
            strcpy(files[file_count].name, entry->d_name);
            
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, entry->d_name);
            
            if (lstat(full_path, &files[file_count].st) == -1) {
                continue;
            }
            
            if (S_ISLNK(files[file_count].st.st_mode)) {
                ssize_t len = readlink(full_path, files[file_count].link_target, 
                                     sizeof(files[file_count].link_target) - 1);
                if (len != -1) {
                    files[file_count].link_target[len] = '\0';
                } else {
                    files[file_count].link_target[0] = '\0';
                }
            } else {
                files[file_count].link_target[0] = '\0';
            }
            
            file_count++;
        }
        
        closedir(dir);
        
        qsort(files, file_count, sizeof(file_info), compare_files);
        
        if (opt_l) {
            print_long_format(files, file_count, opt_a);
        } else {
            print_normal_format(files, file_count, opt_a);
        }
        
        if (dir_idx < dir_count - 1) {
            printf("\n");
        }
    }
    
    return 0;
}