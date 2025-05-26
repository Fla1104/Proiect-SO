#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "treasure.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0 && argc == 3) {
        return add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0 && argc == 3) {
        return list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) {
        return view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc == 4) {
        return remove_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_hunt") == 0 && argc == 3) {
        return remove_hunt(argv[2]);
    } else {
        print_usage();
        return 1;
    }

    return 0;
}
