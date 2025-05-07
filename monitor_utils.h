#ifndef MONITOR_UTILS_H
#define MONITOR_UTILS_H

void list_all_hunts();
int is_monitor_running();
int send_command_to_monitor(const char *command);
int stop_monitor();

#endif
