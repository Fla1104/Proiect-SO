#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "treasure.h"
#include "file_utils.h"
#include "monitor_utils.h"

volatile sig_atomic_t monitor_stopping = 0;
pid_t monitor_pid = -1;
int pipe_fd[2]; 

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


void display_monitor_output() {
    char buffer[4096];
    int bytes_read;
    int flags = fcntl(pipe_fd[0], F_GETFL, 0);
    fcntl(pipe_fd[0], F_SETFL, flags | O_NONBLOCK);
    bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
}

int start_monitor() {
    if (is_monitor_running() || monitor_pid > 0) {
        printf("Monitorul rulează deja!\n");
        return -1;
    }
    if (pipe(pipe_fd) == -1) {
        perror("Eroare la crearea pipe-ului");
        return -1;
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Eroare la crearea procesului monitor");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    } else if (pid == 0) {
        close(pipe_fd[0]); 
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        execl("./monitor_process", "monitor_process", NULL);
        perror("Eroare la execl pentru monitor_process");
        exit(EXIT_FAILURE);
    } else {
        close(pipe_fd[1]);
        printf("Proces monitor pornit cu PID: %d\n", pid);
        monitor_pid = pid;
        sleep(1);
        return 0;
    }
}

void calculate_score() {
    int score_pipe[2];
    if (pipe(score_pipe) == -1) {
        perror("Eroare la crearea pipe-ului pentru calculul scorurilor");
        return;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("Eroare la crearea procesului pentru calculul scorurilor");
        close(score_pipe[0]);
        close(score_pipe[1]);
        return;
    } else if (pid == 0) {
        close(score_pipe[0]); 
        dup2(score_pipe[1], STDOUT_FILENO);
        close(score_pipe[1]);
        execl("./score_calculator", "score_calculator", NULL);
        perror("Eroare la execl pentru score_calculator");
        exit(EXIT_FAILURE);
    } else {
        close(score_pipe[1]); 
        char buffer[4096];
        int bytes_read = read(score_pipe[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("\n=== Scoruri pentru toate vânătorile ===\n");
            printf("%s", buffer);
        } else {
            printf("Nu s-au putut obține scorurile sau nu există vânători.\n");
        }
        close(score_pipe[0]);
        wait(NULL); // Wait for child to finish
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
    printf("  calculate_score - Calculează scorurile utilizatorilor pentru toate vânătorile\n");
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
                usleep(100000); 
                display_monitor_output();
            } else {
                printf("Eroare: Monitorul nu rulează! Folosiți 'start_monitor' pentru a-l porni.\n");
            }
        } else if (strncmp(command, "list_treasures ", 15) == 0) {
            if (is_monitor_running()) {
                send_command_to_monitor(command);
                usleep(100000);
                display_monitor_output();
            } else {
                printf("Eroare: Monitorul nu rulează! Folosiți 'start_monitor' pentru a-l porni.\n");
            }
        } else if (strncmp(command, "view_treasure ", 14) == 0) {
            if (is_monitor_running()) {
                send_command_to_monitor(command);
                usleep(100000);
                display_monitor_output();
            } else {
                printf("Eroare: Monitorul nu rulează! Folosiți 'start_monitor' pentru a-l porni.\n");
            }
        } else if (strcmp(command, "calculate_score") == 0) {
            calculate_score();
        } else if (strcmp(command, "stop_monitor") == 0) {
            if (is_monitor_running()) {
                stop_monitor();
                monitor_stopping = 1;
                close(pipe_fd[0]); // Close the pipe
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
