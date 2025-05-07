#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#include "treasure.h"
#include "file_utils.h"
#include "monitor_utils.h"

#define COMMAND_FILE "/tmp/treasure_monitor_cmd"
#define CONFIG_FILE "/tmp/treasure_monitor_cfg"

volatile sig_atomic_t terminate_flag = 0;

void handle_command(int sig) {
    FILE *cmd_file = fopen(COMMAND_FILE, "r");
    if (cmd_file) {
        char command[256];
        if (fgets(command, sizeof(command), cmd_file)) {
            command[strcspn(command, "\n")] = 0;
            if (strncmp(command, "list_hunts", 10) == 0) {
                list_all_hunts();
            } else if (strncmp(command, "list_treasures", 14) == 0) {
                char hunt_id[MAX_PATH];
                sscanf(command + 15, "%s", hunt_id);
                list_treasures(hunt_id);
            } else if (strncmp(command, "view_treasure", 13) == 0) {
                char hunt_id[MAX_PATH];
                int treasure_id;
                sscanf(command + 14, "%s %d", hunt_id, &treasure_id);
                view_treasure(hunt_id, treasure_id);
            }
        }
        fclose(cmd_file);
    }
}

void handle_terminate(int sig) {
    terminate_flag = 1;
}

int main() {
    struct sigaction sa_cmd;
    memset(&sa_cmd, 0, sizeof(sa_cmd));
    sa_cmd.sa_handler = handle_command;
    sigemptyset(&sa_cmd.sa_mask);
    sigaction(SIGUSR1, &sa_cmd, NULL);
    struct sigaction sa_term;
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = handle_terminate;
    sigemptyset(&sa_term.sa_mask);
    sigaction(SIGTERM, &sa_term, NULL);

    FILE *config_file = fopen(CONFIG_FILE, "w");
    if (config_file) {
        fprintf(config_file, "%d\n", getpid());
        fclose(config_file);
    } else {
        perror("Nu s-a putut crea fișierul de configurare");
        return 1;
    }

    printf("Monitor pornit cu PID %d\n", getpid());
    while (!terminate_flag) {
        sleep(1);
    }
    printf("Monitorul se oprește în 3 secunde...\n");
    usleep(3000000);  
    unlink(COMMAND_FILE);
    unlink(CONFIG_FILE);

    printf("Monitor oprit.\n");
    return 0;
}
