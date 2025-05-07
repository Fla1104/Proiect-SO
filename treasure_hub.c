#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "treasure.h"
#include "file_utils.h"
#include "monitor_utils.h"

volatile sig_atomic_t monitor_stopping = 0;
pid_t monitor_pid = -1;

void handle_child_exit(int sig) {
    int status;
    pid_t child_pid;

    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (child_pid == monitor_pid) {
            if (WIFEXITED(status)) {
                printf("Monitorul s-a terminat cu codul de ieșire: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Monitorul a fost terminat de semnalul: %d\n", WTERMSIG(status));
            }
            monitor_stopping = 0;
            monitor_pid = -1;
        }
    }
}

int start_monitor() {
    if (is_monitor_running() || monitor_pid > 0) {
        printf("Monitorul rulează deja!\n");
        return -1;
    }
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Eroare la crearea procesului monitor");
        return -1;
    } else if (pid == 0) {
        execl("./monitor_process", "monitor_process", NULL);
        perror("Eroare la execl pentru monitor_process");
        exit(EXIT_FAILURE);
    } else {
        printf("Proces monitor pornit cu PID: %d\n", pid);
        monitor_pid = pid;
        sleep(1);
        return 0;
    }
}

int main() {
    char command[256];
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_child_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    printf("\n=== Treasure Hub - Interfață interactivă ===\n");
    printf("Comenzi disponibile:\n");
    printf("  start_monitor - Pornește procesul monitor\n");
    printf("  list_hunts - Listează vânătorile disponibile\n");
    printf("  list_treasures <hunt_id> - Listează comorile dintr-o vânătoare\n");
    printf("  view_treasure <hunt_id> <treasure_id> - Afișează detaliile unei comori\n");
    printf("  stop_monitor - Oprește procesul monitor\n");
    printf("  exit - Ieșire din aplicație\n");
    
    while (1) {
        printf("\n> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        if (monitor_stopping && strcmp(command, "exit") != 0) {
            printf("Monitorul este în proces de oprire, vă rugăm așteptați...\n");
            continue;
        }
        
        if (strcmp(command, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(command, "list_hunts") == 0) {
            if (is_monitor_running()) {
                send_command_to_monitor("list_hunts");
            } else {
                printf("Eroare: Monitorul nu rulează! Folosiți 'start_monitor' pentru a-l porni.\n");
            }
        } else if (strncmp(command, "list_treasures ", 15) == 0) {
            if (is_monitor_running()) {
                send_command_to_monitor(command);
            } else {
                printf("Eroare: Monitorul nu rulează! Folosiți 'start_monitor' pentru a-l porni.\n");
            }
        } else if (strncmp(command, "view_treasure ", 14) == 0) {
            if (is_monitor_running()) {
                send_command_to_monitor(command);
            } else {
                printf("Eroare: Monitorul nu rulează! Folosiți 'start_monitor' pentru a-l porni.\n");
            }
        } else if (strcmp(command, "stop_monitor") == 0) {
            if (is_monitor_running()) {
                stop_monitor();
                monitor_stopping = 1;
            } else {
                printf("Eroare: Monitorul nu rulează!\n");
            }
        } else if (strcmp(command, "exit") == 0) {
            if (is_monitor_running() || monitor_pid > 0) {
                printf("Eroare: Monitorul rulează încă. Opriți-l mai întâi cu 'stop_monitor'.\n");
            } else {
                printf("La revedere!\n");
                break;
            }
        } else if (strlen(command) > 0) {
            printf("Comandă necunoscută. Folosiți una dintre comenzile disponibile.\n");
        }
    }
    
    return 0;
}
