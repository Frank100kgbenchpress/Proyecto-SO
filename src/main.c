#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb_monitor.h"
#include <dirent.h>

void list_usb_devices(const char *mount_point) {
    DIR *dir = opendir(mount_point);
    if (!dir) {
        perror("No se puede abrir el punto de montaje");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            printf(" - %s\n", entry->d_name);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <nombre_memoria_usb>\n", argv[0]);
        return 1;
    }

    const char *usb_name = argv[1];

    char base_path[256];
    snprintf(base_path, sizeof(base_path), "/media/%s", getenv("USER"));

    printf("=== MatCom Guard: Análisis de Seguridad de USB ===\n");

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, usb_name);

    char baseline_file[512];
    snprintf(baseline_file, sizeof(baseline_file),
             "/home/%s/.matcomguard_%s.baseline",
             getenv("USER"),
             usb_name);

    if (access(baseline_file, F_OK) != 0) {
        printf("🛡 No existe baseline. Creándolo por primera vez...\n");
        save_usb_security_baseline(full_path, baseline_file);
    }

    printf("\n🕵️ Comparando estado actual contra baseline...\n");
    compare_usb_security_baseline(full_path, baseline_file, 10.0);

    return 0;
}
