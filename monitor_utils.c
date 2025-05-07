#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

#include "treasure.h"
#include "file_utils.h"
#include "monitor_utils.h"

char treasures_file[PATH_MAX];

void list_all_hunts() {
    DIR *dir;
    struct dirent *entry;
    struct stat dir_stat;

    printf("\n=== Lista vânătorilor disponibile ===\n");
    printf("-------------------------------------------------------\n");
    printf("Nume vânătoare | Număr comori\n");
    printf("-------------------------------------------------------\n");

    dir = opendir(".");
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului curent");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && 
            strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {

	    
            int len = snprintf(treasures_file, sizeof(treasures_file),
                               "%s/treasures.dat", entry->d_name);
            if (len >= sizeof(treasures_file)) {
                fprintf(stderr, "Path trunchiat pentru: %s\n", entry->d_name);
                continue;
            }
            
            if (stat(treasures_file, &dir_stat) == 0) {
                int num_treasures = dir_stat.st_size / sizeof(Treasure);
                printf("%-14s | %d\n", entry->d_name, num_treasures);
            }
        }
    }

    closedir(dir);
}

int is_monitor_running() {
    FILE *config_file = fopen("/tmp/treasure_monitor_cfg", "r");
    if (!config_file) {
        return 0; 
    }

    int monitor_pid;
    fscanf(config_file, "%d", &monitor_pid);
    fclose(config_file);
    if (kill(monitor_pid, 0) == 0) {
        return monitor_pid; 
    }

    return 0;  
}

int send_command_to_monitor(const char *command) {
    int monitor_pid = is_monitor_running();
    if (!monitor_pid) {
        printf("Eroare: Monitorul nu rulează!\n");
        return -1;
    }
    FILE *cmd_file = fopen("/tmp/treasure_monitor_cmd", "w");
    if (!cmd_file) {
        perror("Eroare la deschiderea fișierului de comandă");
        return -1;
    }

    fprintf(cmd_file, "%s\n", command);
    fclose(cmd_file);
    if (kill(monitor_pid, SIGUSR1) != 0) {
        perror("Eroare la trimiterea semnalului către monitor");
        return -1;
    }

    return 0;
}


int stop_monitor() {
    int monitor_pid = is_monitor_running();
    if (!monitor_pid) {
        printf("Eroare: Monitorul nu rulează!\n");
        return -1;
    }
    if (kill(monitor_pid, SIGTERM) != 0) {
        perror("Eroare la trimiterea semnalului de oprire către monitor");
        return -1;
    }

    printf("Semnal de oprire trimis către monitor (PID: %d)\n", monitor_pid);
    return 0;
}
