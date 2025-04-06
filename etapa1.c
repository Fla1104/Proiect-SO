#include <stdio.h>
#include <stdlib.h>

#define USERNAME_MAX 32
#define CLUE_MAX 256

typedef struct {
  int ID;
  char username[USERNAME_MAX];
  float latitudine;
  float longitudine;
  char clue[CLUE_MAX];
  int val;
}Treasure;

int main(int argc,char *argv[]){
  if (argc < 3) {
        fprintf(stderr, "Usage:\n"
                        "--add <hunt_id>\n"
                        "--list <hunt_id>\n"
                        "--view <hunt_id><treasure_id>\n"
                        "--remove_treasure <hunt_id> <treasure_id>\n"
                        "--remove_hunt <hunt_id>\n");
        exit(EXIT_FAILURE);
    }

  return 0;
}
