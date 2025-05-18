#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_HUNT_ID 64
#define MAX_USERNAME 64
#define MAX_CLUE_TEXT 256
#define MAX_PATH 512

typedef struct {
    int treasure_id;
    char username[MAX_USERNAME];
    double latitude;
    double longitude;
    char clue_text[MAX_CLUE_TEXT];
    int value;
} Treasure;

typedef struct {
    char username[MAX_USERNAME];
    int score;
    int treasure_count;
} UserScore;

void calculate_hunt_score(const char *hunt_id) {
    char treasures_file[MAX_PATH];
    snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", hunt_id);
    int fd = open(treasures_file, O_RDONLY);
    if (fd == -1) {
        return;
    }
    Treasure treasure;
    UserScore users[100]; 
    int user_count = 0;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        int user_found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, treasure.username) == 0) {
                // Update existing user's score
                users[i].score += treasure.value;
                users[i].treasure_count++;
                user_found = 1;
                break;
            }
        }
        if (!user_found && user_count < 100) {
            strcpy(users[user_count].username, treasure.username);
            users[user_count].score = treasure.value;
            users[user_count].treasure_count = 1;
            user_count++;
        }
    }
    close(fd);
    if (user_count > 0) {
        printf("\n-- Vânătoarea: %s --\n", hunt_id);
        printf("%-15s | %-10s | %s\n", "Utilizator", "Scor", "Comori");
        printf("----------------------------------------\n");
        for (int i = 0; i < user_count; i++) {
            printf("%-15s | %-10d | %d\n", 
                   users[i].username, users[i].score, users[i].treasure_count);
        }
        printf("\n");
    }
}

int main() {
    DIR *dir;
    struct dirent *entry;
    
    dir = opendir(".");
    if (dir == NULL) {
        fprintf(stderr, "Eroare la deschiderea directorului curent.\n");
        return 1;
    }
    
    int hunt_found = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && 
            strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {
            char treasures_file[MAX_PATH];
            struct stat st;
            snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", entry->d_name);
            if (stat(treasures_file, &st) == 0) {
                calculate_hunt_score(entry->d_name);
                hunt_found = 1;
            }
        }
    }
    
    if (!hunt_found) {
        printf("Nu s-au găsit vânători disponibile.\n");
    }
    
    closedir(dir);
    return 0;
}
