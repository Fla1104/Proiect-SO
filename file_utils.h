#ifndef FILE_UTILS_H
#define FILE_UTILS_H

void log_operation(const char *hunt_id, const char *operation);
void create_symlink(const char *hunt_id);
int create_hunt_directory(const char *hunt_id);

#endif
