#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>

#include "treasure.h"
#include "file_utils.h"

#define COMMAND_FIFO "/tmp/treasure_command_fifo"
#define BUFFER_SIZE 256
#define MAX_HUNT_ID 64
#define PID_FILE "/tmp/monitor.pid"

volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
    running = 0;
}

void process_command(const char *command) {
    if (strncmp(command, "list_hunts", 10) == 0) {
        DIR *dir;
        struct dirent *entry;

        dir = opendir(".");
        if (dir == NULL) {
            fprintf(stdout, "Eroare la deschiderea directorului curent.\n");
            fflush(stdout);
            return;
        }

        fprintf(stdout, "\n=== Vânători disponibile ===\n");
        int count = 0;

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 &&
                strcmp(entry->d_name, "..") != 0) {
                char treasures_file[MAX_PATH];
                struct stat st;

                snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", entry->d_name);
                if (stat(treasures_file, &st) == 0) {
                    fprintf(stdout, "- %s (%ld bytes, %ld treasures)\n",
                            entry->d_name, st.st_size, st.st_size / sizeof(Treasure));
                    count++;
                }
            }
        }

        if (count == 0) {
            fprintf(stdout, "Nu există vânători disponibile.\n");
        }

        closedir(dir);
        fflush(stdout);
    } else if (strncmp(command, "list_treasures ", 15) == 0) {
        char hunt_id[MAX_HUNT_ID];
        sscanf(command + 15, "%s", hunt_id);

        char treasures_file[MAX_PATH];
        snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", hunt_id);

        int fd = open(treasures_file, O_RDONLY);
        if (fd == -1) {
            fprintf(stdout, "Vânătoarea '%s' nu există sau nu conține comori.\n", hunt_id);
            fflush(stdout);
            return;
        }

        struct stat file_stat;
        fstat(fd, &file_stat);

        fprintf(stdout, "\n=== Vânătoarea: %s ===\n", hunt_id);
        fprintf(stdout, "Dimensiunea fișierului: %ld bytes\n", file_stat.st_size);

        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                 localtime(&file_stat.st_mtime));
        fprintf(stdout, "Ultima modificare: %s\n\n", time_str);

        fprintf(stdout, "Lista comorilor:\n");
        fprintf(stdout, "-------------------------------------------------------\n");
        fprintf(stdout, "ID | Utilizator | Latitudine | Longitudine | Valoare\n");
        fprintf(stdout, "-------------------------------------------------------\n");

        Treasure treasure;
        while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
            fprintf(stdout, "%2d | %-10s | %10.6f | %11.6f | %7d\n",
                    treasure.treasure_id, treasure.username,
                    treasure.latitude, treasure.longitude, treasure.value);
        }

        close(fd);
        fflush(stdout);
    } else if (strncmp(command, "view_treasure ", 14) == 0) {
        char hunt_id[MAX_HUNT_ID];
        int treasure_id;
        sscanf(command + 14, "%s %d", hunt_id, &treasure_id);

        char treasures_file[MAX_PATH];
        snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", hunt_id);

        int fd = open(treasures_file, O_RDONLY);
        if (fd == -1) {
            fprintf(stdout, "Vânătoarea '%s' nu există sau nu conține comori.\n", hunt_id);
            fflush(stdout);
            return;
        }

        Treasure treasure;
        int found = 0;

        while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
            if (treasure.treasure_id == treasure_id) {
                found = 1;
                break;
            }
        }

        close(fd);

        if (!found) {
            fprintf(stdout, "Comoara cu ID-ul %d nu a fost găsită în vânătoarea %s\n",
                    treasure_id, hunt_id);
            fflush(stdout);
            return;
        }

        fprintf(stdout, "\n=== Detalii pentru comoara %d ===\n", treasure_id);
        fprintf(stdout, "Utilizator: %s\n", treasure.username);
        fprintf(stdout, "Coordonate GPS: %.6f, %.6f\n", treasure.latitude, treasure.longitude);
        fprintf(stdout, "Indiciu: %s\n", treasure.clue_text);
        fprintf(stdout, "Valoare: %d\n", treasure.value);
        fflush(stdout);
    } else if (strcmp(command, "stop") == 0) {
        running = 0;
    }
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);

    FILE *pid_file = fopen(PID_FILE, "w");
    if (pid_file != NULL) {
        fprintf(pid_file, "%d\n", getpid());
        fclose(pid_file);
    }

    unlink(COMMAND_FIFO);
    if (mkfifo(COMMAND_FIFO, 0666) == -1) {
        perror("Eroare la crearea FIFO-ului de comenzi");
        unlink(PID_FILE);
        return 1;
    }

    fprintf(stdout, "Monitor process started.\n");
    fflush(stdout);

    int fifo_fd = open(COMMAND_FIFO, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Eroare la deschiderea FIFO-ului");
        unlink(COMMAND_FIFO);
        unlink(PID_FILE);
        return 1;
    }

    char buffer[BUFFER_SIZE];

    while (running) {
        int bytes = read(fifo_fd, buffer, BUFFER_SIZE - 1);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            process_command(buffer);
        } else if (bytes == 0) {
            usleep(100000);
            continue;
        } else {
            perror("Eroare la citirea din FIFO");
            break;
        }
    }

    fprintf(stdout, "Monitor process stopping.\n");
    fflush(stdout);

    close(fifo_fd);
    unlink(COMMAND_FIFO);
    unlink(PID_FILE); 
    return 0;
}

