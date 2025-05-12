#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "usb_monitor.h"

void list_usb_devices(const char *mount_point) {
    DIR *dir = opendir(mount_point);
    struct dirent *entry;

    if (!dir) {
        perror("No se puede abrir el punto de montaje");
        return;
    }

    printf("📦 Dispositivos montados en %s:\n", mount_point);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            printf(" - %s\n", entry->d_name);
        }
    }

    closedir(dir);
}