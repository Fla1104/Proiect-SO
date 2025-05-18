#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>


#include "treasure.h"
#include "file_utils.h"

int add_treasure(const char *hunt_id) {
    if (create_hunt_directory(hunt_id) == -1) {
        return -1;
    }

    Treasure new_treasure;
    char treasures_file[MAX_PATH];
    int fd, max_id = 0;

    snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", hunt_id);

    fd = open(treasures_file, O_RDONLY);
    if (fd != -1) {
        Treasure temp;
        while (read(fd, &temp, sizeof(Treasure)) == sizeof(Treasure)) {
            if (temp.treasure_id > max_id) {
                max_id = temp.treasure_id;
            }
        }
        close(fd);
    }

    new_treasure.treasure_id = max_id + 1;

    printf("\n--- Adăugare comoară nouă ---\n");
    printf("Introduceți numele utilizatorului: ");
    scanf("%63s", new_treasure.username);

    printf("Introduceți coordonatele GPS (latitudine): ");
    scanf("%lf", &new_treasure.latitude);

    printf("Introduceți coordonatele GPS (longitudine): ");
    scanf("%lf", &new_treasure.longitude);

    printf("Introduceți textul indiciului: ");
    getchar();
    fgets(new_treasure.clue_text, MAX_CLUE_TEXT, stdin);
    new_treasure.clue_text[strcspn(new_treasure.clue_text, "\n")] = 0;

    printf("Introduceți valoarea comorii: ");
    scanf("%d", &new_treasure.value);

    fd = open(treasures_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Eroare la deschiderea fișierului de comori");
        return -1;
    }

    if (write(fd, &new_treasure, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Eroare la scrierea comorii în fișier");
        close(fd);
        return -1;
    }

    close(fd);

    char log_msg[MAX_PATH];
    snprintf(log_msg, MAX_PATH, "Adăugare comoară cu ID %d în vânătoarea %s", new_treasure.treasure_id, hunt_id);
    log_operation(hunt_id, log_msg);
    create_symlink(hunt_id);

    printf("\nComoară adăugată cu succes cu ID-ul %d\n", new_treasure.treasure_id);
    return 0;
}

int list_treasures(const char *hunt_id) {
    char treasures_file[MAX_PATH];
    struct stat file_stat;
    int fd;

    snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", hunt_id);

    if (stat(treasures_file, &file_stat) == -1) {
        printf("Vânătoarea '%s' nu există sau nu conține comori.\n", hunt_id);
        return -1;
    }

    printf("\n=== Vânătoarea: %s ===\n", hunt_id);
    printf("Dimensiunea fișierului: %ld bytes\n", file_stat.st_size);

    char time_str[100];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
    printf("Ultima modificare: %s\n\n", time_str);

    fd = open(treasures_file, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fișierului de comori");
        return -1;
    }

    printf("Lista comorilor:\n");
    printf("-------------------------------------------------------\n");
    printf("ID | Utilizator | Latitudine | Longitudine | Valoare\n");
    printf("-------------------------------------------------------\n");

    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("%2d | %-10s | %10.6f | %11.6f | %7d\n",
               treasure.treasure_id, treasure.username,
               treasure.latitude, treasure.longitude, treasure.value);
    }

    close(fd);
    log_operation(hunt_id, "Listare comori");
    return 0;
}

int view_treasure(const char *hunt_id, int treasure_id) {
    char treasures_file[MAX_PATH];
    int fd;

    snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", hunt_id);

    fd = open(treasures_file, O_RDONLY);
    if (fd == -1) {
        printf("Vânătoarea '%s' nu există sau nu conține comori.\n", hunt_id);
        return -1;
    }

    Treasure treasure;
    int found = 0;

    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (treasure.treasure_id == treasure_id) {
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found) {
        printf("Comoara cu ID-ul %d nu a fost găsită în vânătoarea %s\n", treasure_id, hunt_id);
        return -1;
    }

    printf("\n=== Detalii pentru comoara %d ===\n", treasure_id);
    printf("Utilizator: %s\n", treasure.username);
    printf("Coordonate GPS: %.6f, %.6f\n", treasure.latitude, treasure.longitude);
    printf("Indiciu: %s\n", treasure.clue_text);
    printf("Valoare: %d\n", treasure.value);

    char log_msg[MAX_PATH];
    snprintf(log_msg, MAX_PATH, "Vizualizare comoară cu ID %d", treasure_id);
    log_operation(hunt_id, log_msg);

    return 0;
}

int remove_treasure(const char *hunt_id, int treasure_id) {
    char treasures_file[MAX_PATH];
    char temp_file[MAX_PATH];
    int fd_src, fd_dst;

    snprintf(treasures_file, MAX_PATH, "%s/treasures.dat", hunt_id);
    snprintf(temp_file, MAX_PATH, "%s/temp.dat", hunt_id);

    fd_src = open(treasures_file, O_RDONLY);
    if (fd_src == -1) {
        printf("Vânătoarea '%s' nu există sau nu conține comori.\n", hunt_id);
        return -1;
    }

    fd_dst = open(temp_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_dst == -1) {
        perror("Eroare la crearea fișierului temporar");
        close(fd_src);
        return -1;
    }

    Treasure treasure;
    int found = 0;

    while (read(fd_src, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (treasure.treasure_id == treasure_id) {
            found = 1;
            continue;
        }
        if (write(fd_dst, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
            perror("Eroare la scrierea în fișierul temporar");
            close(fd_src);
            close(fd_dst);
            return -1;
        }
    }

    close(fd_src);
    close(fd_dst);

    if (!found) {
        unlink(temp_file);
        printf("Comoara cu ID-ul %d nu a fost găsită în vânătoarea %s\n", treasure_id, hunt_id);
        return -1;
    }

    if (rename(temp_file, treasures_file) == -1) {
        perror("Eroare la redenumirea fișierului");
        return -1;
    }

    char log_msg[MAX_PATH];
    snprintf(log_msg, MAX_PATH, "Ștergere comoară cu ID %d", treasure_id);
    log_operation(hunt_id, log_msg);
    create_symlink(hunt_id);

    printf("Comoara cu ID-ul %d a fost ștearsă cu succes\n", treasure_id);
    return 0;
}

int remove_hunt(const char *hunt_id) {
    struct stat st = {0};
    if (stat(hunt_id, &st) == -1) {
        printf("Vânătoarea '%s' nu există.\n", hunt_id);
        return -1;
    }

    char command[MAX_PATH * 2];
    snprintf(command, sizeof(command), "rm -rf %s", hunt_id);
    if (system(command) != 0) {
        perror("Eroare la ștergerea directorului de vânătoare");
        return -1;
    }

    char symlink_path[MAX_PATH];
    snprintf(symlink_path, MAX_PATH, "logged_hunt-%s", hunt_id);
    unlink(symlink_path);

    printf("Vânătoarea '%s' a fost ștearsă cu succes\n", hunt_id);
    return 0;
}

void print_usage() {
    printf("\n===== Treasure Manager - Sistem de vânătoare de comori =====\n");
    printf("Utilizare:\n");
    printf("  treasure_manager --add <hunt_id>\n");
    printf("      Adaugă o nouă comoară în vânătoarea specificată\n\n");
    printf("  treasure_manager --list <hunt_id>\n");
    printf("      Listează toate comorile din vânătoarea specificată\n\n");
    printf("  treasure_manager --view <hunt_id> <treasure_id>\n");
    printf("      Vizualizează detaliile unei comori specifice\n\n");
    printf("  treasure_manager --remove_treasure <hunt_id> <treasure_id>\n");
    printf("      Șterge o comoară specifică din vânătoarea specificată\n\n");
    printf("  treasure_manager --remove_hunt <hunt_id>\n");
    printf("      Șterge o întreagă vânătoare\n\n");
}
