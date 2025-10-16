#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <errno.h>

#define MAX_NAME_LEN 256
#define BUFFER_SIZE 4096

struct file_header {
    char name[MAX_NAME_LEN];             // имя файла (нулевой терминатор)
    off_t size;                         // размер файла
    mode_t mode;                        // права доступа
    uid_t uid;                         // владелец
    gid_t gid;                         // группа
    struct timespec atime;             // время последнего доступа
    struct timespec mtime;             // время последней модификации
    char deleted;                      // флаг удаления (0 - в архиве, 1 - удалён)
};

void print_help() {
    printf("Usage:\n");
    printf("  ./archiver arch_name -i file1     Add file to archive\n");
    printf("  ./archiver arch_name -e file1     Extract file from archive and remove\n");
    printf("  ./archiver arch_name -s            Show archive status\n");
    printf("  ./archiver -h                      Show help\n");
}

int read_header(int fd, struct file_header *hdr) {
    ssize_t n = read(fd, hdr, sizeof(struct file_header));
    if (n == 0) return 0;
    if (n != sizeof(struct file_header)) {
        perror("read header");
        return -1;
    }
    return 1;
}

int write_header(int fd, struct file_header *hdr) {
    ssize_t n = write(fd, hdr, sizeof(struct file_header));
    if (n != sizeof(struct file_header)) {
        perror("write header");
        return -1;
    }
    return 0;
}

int add_file(const char *archive_name, const char *filename) {
    int a_fd = open(archive_name, O_RDWR | O_CREAT, 0644);
    if (a_fd < 0) {
        perror("open archive");
        return 1;
    }

    if (lseek(a_fd, 0, SEEK_END) == (off_t)-1) {
        perror("lseek");
        close(a_fd);
        return 1;
    }

    struct stat st;
    if (stat(filename, &st) < 0) {
        perror("stat input file");
        close(a_fd);
        return 1;
    }

    struct file_header hdr = {0};
    strncpy(hdr.name, filename, MAX_NAME_LEN - 1);
    hdr.size = st.st_size;
    hdr.mode = st.st_mode;
    hdr.uid = st.st_uid;
    hdr.gid = st.st_gid;
    hdr.atime = st.st_atim;
    hdr.mtime = st.st_mtim;
    hdr.deleted = 0;

    if (write_header(a_fd, &hdr) < 0) {
        close(a_fd);
        return 1;
    }

    int f_fd = open(filename, O_RDONLY);
    if (f_fd < 0) {
        perror("open input file");
        close(a_fd);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t rd;
    while ((rd = read(f_fd, buffer, BUFFER_SIZE)) > 0) {
        if (write(a_fd, buffer, rd) != rd) {
            perror("write to archive");
            close(f_fd);
            close(a_fd);
            return 1;
        }
    }
    if (rd < 0) {
        perror("read input file");
        close(f_fd);
        close(a_fd);
        return 1;
    }

    close(f_fd);
    close(a_fd);
    return 0;
}

int extract_file(const char *archive_name, const char *filename) {
    int a_fd = open(archive_name, O_RDWR);
    if (a_fd < 0) {
        perror("open archive");
        return 1;
    }

    off_t pos = 0;
    int found = 0;
    struct file_header hdr;
    while (1) {
        ssize_t res = pread(a_fd, &hdr, sizeof(hdr), pos);
        if (res == 0) break;
        if (res != sizeof(hdr)) {
            perror("read header");
            close(a_fd);
            return 1;
        }

        off_t data_pos = pos + sizeof(hdr);

        if (hdr.deleted == 0 && strcmp(hdr.name, filename) == 0) {
            int f_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, hdr.mode);
            if (f_fd < 0) {
                perror("create output file");
                close(a_fd);
                return 1;
            }

            off_t to_read = hdr.size;
            char buf[BUFFER_SIZE];
            while (to_read > 0) {
                ssize_t chunk = to_read > BUFFER_SIZE ? BUFFER_SIZE : to_read;
                ssize_t rd = pread(a_fd, buf, chunk, data_pos);
                if (rd <= 0) {
                    perror("read file data");
                    close(f_fd);
                    close(a_fd);
                    return 1;
                }
                if (write(f_fd, buf, rd) != rd) {
                    perror("write output file");
                    close(f_fd);
                    close(a_fd);
                    return 1;
                }
                data_pos += rd;
                to_read -= rd;
            }
            close(f_fd);

            struct timespec times[2] = {hdr.atime, hdr.mtime};
            if (utimensat(AT_FDCWD, filename, times, 0) < 0) {
                perror("utimensat");
            }
            if (chmod(filename, hdr.mode) < 0) {
                perror("chmod");
            }

            hdr.deleted = 1;
            if (pwrite(a_fd, &hdr, sizeof(hdr), pos) != sizeof(hdr)) {
                perror("mark deleted");
                close(a_fd);
                return 1;
            }

            found = 1;
            break;
        }

        pos += sizeof(hdr) + hdr.size;
    }

    if (!found) {
        printf("File '%s' not found in archive.\n", filename);
    }
    close(a_fd);
    return found ? 0 : 1;
}

int print_stat(const char *archive_name) {
    int a_fd = open(archive_name, O_RDONLY);
    if (a_fd < 0) {
        perror("open archive");
        return 1;
    }

    off_t pos = 0;
    struct file_header hdr;
    printf("Archive '%s' contents:\n", archive_name);
    printf("%-30s  Size      Mode\n", "Name");
    while (1) {
        ssize_t res = pread(a_fd, &hdr, sizeof(hdr), pos);
        if (res == 0) break;
        if (res != sizeof(hdr)) {
            perror("read header");
            close(a_fd);
            return 1;
        }
        if (!hdr.deleted) printf("%-30s  %-8ld  %04o\n", hdr.name, (long)hdr.size, hdr.mode & 0777);
        pos += sizeof(hdr) + hdr.size;
    }
    close(a_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    if (argc < 3) {
        print_help();
        return 1;
    }

    const char *archive_name = argv[1];
    const char *option = argv[2];

    if ((strcmp(option, "-i") == 0 || strcmp(option, "--input") == 0) && argc >= 4) {
        return add_file(archive_name, argv[3]);
    } else if ((strcmp(option, "-e") == 0 || strcmp(option, "--extract") == 0) && argc >= 4) {
        return extract_file(archive_name, argv[3]);
    } else if (strcmp(option, "-s") == 0 || strcmp(option, "--stat") == 0) {
        return print_stat(archive_name);
    } else {
        print_help();
        return 1;
    }
}
