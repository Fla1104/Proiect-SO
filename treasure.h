#ifndef TREASURE_H
#define TREASURE_H

#define MAX_PATH 256
#define MAX_CLUE_TEXT 512
#define MAX_USERNAME 64

typedef struct {
    int treasure_id;
    char username[MAX_USERNAME];
    double latitude;
    double longitude;
    char clue_text[MAX_CLUE_TEXT];
    int value;
} Treasure;

int add_treasure(const char *hunt_id);
int list_treasures(const char *hunt_id);
int view_treasure(const char *hunt_id, int treasure_id);
int remove_treasure(const char *hunt_id, int treasure_id);
int remove_hunt(const char *hunt_id);
void print_usage();

#endif
