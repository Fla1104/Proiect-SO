#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#include "monitor_utils.h"

#define COMMAND_FIFO "/tmp/treasure_command_fifo"
#define PID_FILE "/tmp/monitor.pid"

int is_monitor_running() {
    FILE *pid_file = fopen(PID_FILE, "r");
    if (!pid_file) {
        return 0;  
    }

    int pid;
    if (fscanf(pid_file, "%d", &pid) != 1) {
        fclose(pid_file);
        return 0;
    }
    fclose(pid_file);

    
    if (kill(pid, 0) == 0) {
        return pid;
    } else {
        return 0;  
    }
}

int send_command_to_monitor(const char *command) {
    int fd = open(COMMAND_FIFO, O_WRONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea FIFO-ului de comenzi");
        return -1;
    }

    if (write(fd, command, strlen(command)) == -1) {
        perror("Eroare la scriere în FIFO");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int stop_monitor() {
    int pid = is_monitor_running();
    if (!pid) {
        printf("Eroare: Monitorul nu rulează!\n");
        return -1;
    }

    if (kill(pid, SIGTERM) != 0) {
        perror("Eroare la trimiterea semnalului de oprire către monitor");
        return -1;
    }

    printf("Semnal de oprire trimis către monitor (PID: %d)\n", pid);
    return 0;
}
