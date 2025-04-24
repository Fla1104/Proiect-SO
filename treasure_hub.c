#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define MAX_COMMAND 256
#define MAX_PATH 256
#define COMMAND_FILE "monitor_command.tmp"

pid_t monitor_pid = -1;
int monitor_active = 0;

typedef struct {
    int treasure_id;
    char username[64];
    double latitude;
    double longitude;
    char clue_text[512];
    int value;
} Treasure;

void list_hunts() {
    DIR *dir;
    struct dirent *entry;
    
    dir = opendir(".");
    if (!dir) {
        perror("Eroare la deschiderea directorului curent");
        return;
    }
    
    printf("\n=== Lista vânătorilor de comori ===\n");
    printf("Nume vânătoare | Număr de comori\n");
    printf("---------------------------\n");
    
    int hunt_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && 
            strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {
            
            char treasures_path[MAX_PATH];
            struct stat st;
            
            snprintf(treasures_path, MAX_PATH, "%s/treasures.dat", entry->d_name);
            
            if (stat(treasures_path, &st) == 0) {
                int count = 0;
                int fd = open(treasures_path, O_RDONLY);
                if (fd != -1) {
                    Treasure treasure;
                    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
                        count++;
                    }
                    close(fd);
                }
                
                printf("%-14s | %d\n", entry->d_name, count);
                hunt_count++;
            }
        }
    }
    
    if (hunt_count == 0) {
        printf("Nu există vânători de comori.\n");
    }
    
    closedir(dir);
}

void handle_sigchld(int sig) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    
    if (pid == monitor_pid) {
        printf("Procesul monitor s-a terminat cu statusul: ");
        if (WIFEXITED(status)) {
            printf("exit cu codul %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("terminat de semnalul %d\n", WTERMSIG(status));
        }
        
        monitor_pid = -1;
        monitor_active = 0;
    }
}

void handle_sigusr1(int sig) {
    FILE *cmd_file = fopen(COMMAND_FILE, "r");
    if (!cmd_file) {
        return;
    }
    
    char command[MAX_COMMAND];
    if (fgets(command, MAX_COMMAND, cmd_file) == NULL) {
        fclose(cmd_file);
        return;
    }
    fclose(cmd_file);
    
    command[strcspn(command, "\n")] = 0;
    
    if (strcmp(command, "list_hunts") == 0) {
        list_hunts();
    }
}

void start_monitor() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    while (1) {
        pause();
    }
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    char cmd[MAX_COMMAND];
    
    printf("=== Treasure Hub - Sistem de vânătoare de comori ===\n");
    printf("Comenzi disponibile:\n");
    printf("  start_monitor: pornește procesul de monitorizare\n");
    printf("  list_hunts: listează toate vânătorile și numărul de comori\n");
    printf("  exit: închide programul\n");
    
    while (1) {
        printf("\n> ");
        fgets(cmd, MAX_COMMAND, stdin);
        cmd[strcspn(cmd, "\n")] = 0;  // Eliminăm newline
        
        if (strcmp(cmd, "exit") == 0) {
            if (monitor_active) {
                printf("Eroare: procesul de monitorizare încă rulează. Opriți-l mai întâi.\n");
            } else {
                printf("La revedere!\n");
                break;
            }
        } else if (strcmp(cmd, "start_monitor") == 0) {
            if (monitor_active) {
                printf("Procesul de monitorizare rulează deja.\n");
                continue;
            }
            pid_t pid = fork();
            
            if (pid == -1) {
                perror("Eroare la fork");
                continue;
            } else if (pid == 0) {
                start_monitor();
                exit(0);
            } else {
                monitor_pid = pid;
                monitor_active = 1;
                printf("Procesul de monitorizare a fost pornit (PID: %d).\n", pid);
            }
        } else if (strcmp(cmd, "list_hunts") == 0) {
            if (!monitor_active) {
                printf("Eroare: procesul de monitorizare nu rulează. Porniți-l mai întâi.\n");
                continue;
            }
            FILE *cmd_file = fopen(COMMAND_FILE, "w");
            if (cmd_file) {
                fprintf(cmd_file, "list_hunts\n");
                fclose(cmd_file);
                kill(monitor_pid, SIGUSR1);
            }
        } else {
            printf("Comandă necunoscută: %s\n", cmd);
        }
    }
    
    unlink(COMMAND_FILE);
    
    return 0;
}
