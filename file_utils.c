#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "treasure.h"
#include "file_utils.h"

void log_operation(const char *hunt_id, const char *operation) {
    char log_path[MAX_PATH];
    char time_str[100];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    snprintf(log_path, MAX_PATH, "%s/logged_hunt", hunt_id);
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Eroare la deschiderea fișierului de log");
        return;
    }

    char log_entry[MAX_PATH + 100];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", time_str, operation);
    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);
}

void create_symlink(const char *hunt_id) {
    char log_path[MAX_PATH];
    char symlink_path[MAX_PATH];

    snprintf(log_path, MAX_PATH, "%s/logged_hunt", hunt_id);
    snprintf(symlink_path, MAX_PATH, "logged_hunt-%s", hunt_id);

    unlink(symlink_path);
    if (symlink(log_path, symlink_path) == -1) {
        perror("Eroare la crearea link-ului simbolic");
    }
}

int create_hunt_directory(const char *hunt_id) {
    struct stat st = {0};
    if (stat(hunt_id, &st) == -1) {
        if (mkdir(hunt_id, 0755) == -1) {
            perror("Eroare la crearea directorului de vânătoare");
            return -1;
        }
        printf("Directorul vânătorii '%s' a fost creat.\n", hunt_id);
    }
    return 0;
}
