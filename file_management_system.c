/*
 * Assignment: Design and Implementation of a File Management System Using C
 * Uses POSIX / Linux system calls:
 *   open(), read(), write(), close(), unlink(), rename(), fstat(), stat(), chmod()
 *
 * Compilation:
 *   gcc -Wall -Wextra -std=c11 -o fms file_management_system.c
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PATH_LEN 256
#define IO_BUF_SIZE  4096
#define LINE_BUF_SIZE 1024

/* Print formatted system error message using errno */
static void print_sys_error(const char *action, const char *target) {
    fflush(stdout);
    fprintf(stderr, "[ERROR] %s '%s' failed: %s (errno=%d)\n",
            action, target, strerror(errno), errno);
}

/* Strip trailing newline characters (\r or \n) from fgets buffer */
static void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

/* Read a non-empty filename/input line safely from stdin */
static int read_line(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, (int)size, stdin) == NULL) {
        return -1;
    }
    trim_newline(buf);
    return 0;
}

/* Write all bytes in buf to fd, handling partial writes and EINTR */
static ssize_t write_all(int fd, const char *buf, size_t count) {
    size_t total_written = 0;
    while (total_written < count) {
        ssize_t n = write(fd, buf + total_written, count - total_written);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        total_written += (size_t)n;
    }
    return (ssize_t)total_written;
}

/* Format st_mode permissions into rwxrwxrwx string */
static void format_permissions(mode_t mode, char *out) {
    out[0] = (mode & S_IRUSR) ? 'r' : '-';
    out[1] = (mode & S_IWUSR) ? 'w' : '-';
    out[2] = (mode & S_IXUSR) ? 'x' : '-';
    out[3] = (mode & S_IRGRP) ? 'r' : '-';
    out[4] = (mode & S_IWGRP) ? 'w' : '-';
    out[5] = (mode & S_IXGRP) ? 'x' : '-';
    out[6] = (mode & S_IROTH) ? 'r' : '-';
    out[7] = (mode & S_IWOTH) ? 'w' : '-';
    out[8] = (mode & S_IXOTH) ? 'x' : '-';
    out[9] = '\0';
}

/* Display file size using fstat() on an open file descriptor */
static void display_file_size(int fd, const char *filename) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        print_sys_error("Checking size of", filename);
        return;
    }
    printf("[INFO] Current size of '%s': %lld bytes\n",
           filename, (long long)st.st_size);
}

/* Accept multi-line user text until a line containing only "END" is entered */
static int write_user_text_to_fd(int fd, const char *filename) {
    char line[LINE_BUF_SIZE];
    printf("Enter text (type 'END' on a new line to finish):\n");

    while (1) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        /* Check if line is the sentinel "END" */
        char check[LINE_BUF_SIZE];
        strncpy(check, line, sizeof(check) - 1);
        check[sizeof(check) - 1] = '\0';
        trim_newline(check);

        if (strcmp(check, "END") == 0) {
            break;
        }

        size_t len = strlen(line);
        if (write_all(fd, line, len) == -1) {
            print_sys_error("Writing to", filename);
            return -1;
        }
    }
    return 0;
}

/* 1. Create a new file using open(O_CREAT | O_EXCL) */
static void create_file(void) {
    char filename[MAX_PATH_LEN];
    if (read_line("Enter new filename to create: ", filename, sizeof(filename)) != 0 ||
        filename[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid filename.\n");
        return;
    }

    int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd == -1) {
        print_sys_error("Creating", filename);
        return;
    }

    printf("[SUCCESS] File '%s' created successfully (fd=%d, mode=0644).\n",
           filename, fd);

    if (close(fd) == -1) {
        print_sys_error("Closing", filename);
    }
}

/* 2. Write (overwrite) text to an existing file and display updated size */
static void write_to_file(void) {
    char filename[MAX_PATH_LEN];
    if (read_line("Enter filename to write to: ", filename, sizeof(filename)) != 0 ||
        filename[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid filename.\n");
        return;
    }

    int fd = open(filename, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        print_sys_error("Opening for write", filename);
        return;
    }

    if (write_user_text_to_fd(fd, filename) == 0) {
        printf("[SUCCESS] Data written to '%s'.\n", filename);
        display_file_size(fd, filename);
    }

    if (close(fd) == -1) {
        print_sys_error("Closing", filename);
    }
}

/* 3. Read and display contents of an existing file */
static void read_file(void) {
    char filename[MAX_PATH_LEN];
    if (read_line("Enter filename to read: ", filename, sizeof(filename)) != 0 ||
        filename[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid filename.\n");
        return;
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        print_sys_error("Opening for read", filename);
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == 0 && S_ISDIR(st.st_mode)) {
        fprintf(stderr, "[ERROR] '%s' is a directory, not a regular file.\n", filename);
        close(fd);
        return;
    }

    printf("\n--- Contents of '%s' ---\n", filename);
    fflush(stdout);
    char buffer[IO_BUF_SIZE];
    ssize_t bytes_read;
    ssize_t total_bytes = 0;
    char last_char = '\n';

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (write_all(STDOUT_FILENO, buffer, (size_t)bytes_read) == -1) {
            print_sys_error("Writing output for", filename);
            close(fd);
            return;
        }
        last_char = buffer[bytes_read - 1];
        total_bytes += bytes_read;
    }

    if (bytes_read == -1) {
        print_sys_error("Reading", filename);
        close(fd);
        return;
    }

    if (total_bytes == 0) {
        printf("(File is empty)\n");
    } else if (last_char != '\n') {
        /* Ensure clean prompt formatting when file does not end with newline */
        printf("\n");
    }
    printf("--- End of '%s' (%zd bytes) ---\n", filename, total_bytes);

    if (close(fd) == -1) {
        print_sys_error("Closing", filename);
    }
}

/* 4. Append new content to an existing file and display updated size */
static void append_to_file(void) {
    char filename[MAX_PATH_LEN];
    if (read_line("Enter filename to append to: ", filename, sizeof(filename)) != 0 ||
        filename[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid filename.\n");
        return;
    }

    int fd = open(filename, O_WRONLY | O_APPEND);
    if (fd == -1) {
        print_sys_error("Opening for append", filename);
        return;
    }

    if (write_user_text_to_fd(fd, filename) == 0) {
        printf("[SUCCESS] Data appended to '%s'.\n", filename);
        display_file_size(fd, filename);
    }

    if (close(fd) == -1) {
        print_sys_error("Closing", filename);
    }
}

/* 5. Copy contents of source file to destination file */
static void copy_file(void) {
    char src[MAX_PATH_LEN];
    char dest[MAX_PATH_LEN];

    if (read_line("Enter source filename: ", src, sizeof(src)) != 0 || src[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid source filename.\n");
        return;
    }
    if (read_line("Enter destination filename: ", dest, sizeof(dest)) != 0 || dest[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid destination filename.\n");
        return;
    }

    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        print_sys_error("Opening source", src);
        return;
    }

    struct stat src_st;
    if (fstat(src_fd, &src_st) == -1) {
        print_sys_error("Inspecting source", src);
        close(src_fd);
        return;
    }
    if (S_ISDIR(src_st.st_mode)) {
        fprintf(stderr, "[ERROR] Source '%s' is a directory.\n", src);
        close(src_fd);
        return;
    }

    /* Prevent self-overwrite truncation */
    struct stat dest_st;
    if (stat(dest, &dest_st) == 0 &&
        src_st.st_dev == dest_st.st_dev &&
        src_st.st_ino == dest_st.st_ino) {
        fprintf(stderr, "[ERROR] Source and destination refer to the same file.\n");
        close(src_fd);
        return;
    }

    mode_t dest_mode = (src_st.st_mode & 0777) ? (src_st.st_mode & 0777) : 0644;
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, dest_mode);
    if (dest_fd == -1) {
        print_sys_error("Opening destination", dest);
        close(src_fd);
        return;
    }

    char buffer[IO_BUF_SIZE];
    ssize_t bytes_read;
    size_t total_copied = 0;

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write_all(dest_fd, buffer, (size_t)bytes_read) == -1) {
            print_sys_error("Writing destination", dest);
            close(src_fd);
            close(dest_fd);
            return;
        }
        total_copied += (size_t)bytes_read;
    }

    if (bytes_read == -1) {
        print_sys_error("Reading source", src);
    } else {
        printf("[SUCCESS] Copied '%s' -> '%s' (%zu bytes).\n", src, dest, total_copied);
    }

    if (close(src_fd) == -1) {
        print_sys_error("Closing source", src);
    }
    if (close(dest_fd) == -1) {
        print_sys_error("Closing destination", dest);
    }
}

/* 6. Rename an existing file using rename() */
static void rename_file(void) {
    char old_name[MAX_PATH_LEN];
    char new_name[MAX_PATH_LEN];

    if (read_line("Enter current filename: ", old_name, sizeof(old_name)) != 0 ||
        old_name[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid current filename.\n");
        return;
    }
    if (read_line("Enter new filename: ", new_name, sizeof(new_name)) != 0 ||
        new_name[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid new filename.\n");
        return;
    }

    if (rename(old_name, new_name) == -1) {
        print_sys_error("Renaming", old_name);
        return;
    }

    printf("[SUCCESS] File renamed from '%s' to '%s'.\n", old_name, new_name);
}

/* 7. Delete an existing file using unlink() */
static void delete_file(void) {
    char filename[MAX_PATH_LEN];
    if (read_line("Enter filename to delete: ", filename, sizeof(filename)) != 0 ||
        filename[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid filename.\n");
        return;
    }

    if (unlink(filename) == -1) {
        print_sys_error("Deleting", filename);
        return;
    }

    printf("[SUCCESS] File '%s' deleted successfully.\n", filename);
}

/* 8. Change file permissions using chmod() (Part C) */
static void change_permissions(void) {
    char filename[MAX_PATH_LEN];
    char mode_str[32];

    if (read_line("Enter filename to change permissions: ", filename, sizeof(filename)) != 0 ||
        filename[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid filename.\n");
        return;
    }

    /* Check file exists before changing mode and display current permissions */
    struct stat st;
    if (stat(filename, &st) == -1) {
        print_sys_error("Accessing", filename);
        return;
    }

    char perm_str[10];
    format_permissions(st.st_mode, perm_str);
    printf("[INFO] Current permissions of '%s': %04o (%s)\n",
           filename, (unsigned int)(st.st_mode & 0777), perm_str);

    if (read_line("Enter new permissions in octal (e.g., 0644, 0755, 0444): ",
                  mode_str, sizeof(mode_str)) != 0 ||
        mode_str[0] == '\0') {
        fprintf(stderr, "[ERROR] Invalid permission input.\n");
        return;
    }

    char *endptr = NULL;
    errno = 0;
    long parsed_mode = strtol(mode_str, &endptr, 8);
    if (errno != 0 || endptr == mode_str || *endptr != '\0' ||
        parsed_mode < 0 || parsed_mode > 07777) {
        fprintf(stderr, "[ERROR] Invalid octal permission '%s'. Use values like 0644 or 755.\n",
                mode_str);
        return;
    }

    if (chmod(filename, (mode_t)parsed_mode) == -1) {
        print_sys_error("Changing permissions of", filename);
        return;
    }

    if (stat(filename, &st) == 0) {
        format_permissions(st.st_mode, perm_str);
        printf("[SUCCESS] Permissions of '%s' updated to %04o (%s).\n",
               filename, (unsigned int)(st.st_mode & 0777), perm_str);
    } else {
        printf("[SUCCESS] Permissions of '%s' updated to %04lo.\n", filename, parsed_mode);
    }
}

static void print_menu(void) {
    printf("\n============================================\n");
    printf("   Linux File Management System (Syscalls)\n");
    printf("============================================\n");
    printf(" 1. Create File\n");
    printf(" 2. Write to File\n");
    printf(" 3. Read File\n");
    printf(" 4. Append to File\n");
    printf(" 5. Copy File\n");
    printf(" 6. Rename File\n");
    printf(" 7. Delete File\n");
    printf(" 8. Change File Permissions (chmod)\n");
    printf(" 9. Exit\n");
    printf("--------------------------------------------\n");
}

int main(void) {
    char choice_buf[32];
    int choice = 0;

    while (1) {
        print_menu();
        if (read_line("Enter your choice (1-9): ", choice_buf, sizeof(choice_buf)) != 0) {
            printf("\n[INFO] End of input detected. Exiting.\n");
            break;
        }

        char *endptr = NULL;
        long parsed = strtol(choice_buf, &endptr, 10);
        if (choice_buf[0] == '\0' || *endptr != '\0') {
            fprintf(stderr, "[ERROR] Please enter a valid number between 1 and 9.\n");
            continue;
        }
        choice = (int)parsed;

        switch (choice) {
            case 1:
                create_file();
                break;
            case 2:
                write_to_file();
                break;
            case 3:
                read_file();
                break;
            case 4:
                append_to_file();
                break;
            case 5:
                copy_file();
                break;
            case 6:
                rename_file();
                break;
            case 7:
                delete_file();
                break;
            case 8:
                change_permissions();
                break;
            case 9:
                printf("[INFO] Exiting File Management System. Goodbye!\n");
                return 0;
            default:
                fprintf(stderr, "[ERROR] Invalid choice (%d). Select 1 to 9.\n", choice);
                break;
        }
    }

    return 0;
}
