#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "usb_monitor.h"
#include <time.h>
struct stat_snapshot {
    char path[1024];
    long mtime;
};
void recursive_compare(const char *cur_path,
                       const char *snapshot_file,
                       struct stat_snapshot *old_files,
                       int old_count,
                       int *found) {
    DIR *d = opendir(cur_path);
    struct dirent *entry;

    if (!d) return;

    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", cur_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                recursive_compare(full_path, snapshot_file, old_files, old_count, found);
            } else {
                int match = 0;
                for (int i = 0; i < old_count; i++) {
                    if (strcmp(full_path, old_files[i].path) == 0) {
                        match = 1;
                        found[i] = 1;
                        if (st.st_mtime != old_files[i].mtime) {
                            printf("📝 Modificado: %s\n", full_path);
                        }
                        break;
                    }
                }
                if (!match) {
                    printf("➕ Nuevo: %s\n", full_path);
                }
            }
        }
    }

    closedir(d);
}
void compare_usb_snapshot(const char *path, const char *snapshot_file) {
    FILE *in = fopen(snapshot_file, "r");
    if (!in) {
        printf("⚠️ No se encontró snapshot anterior. Generando uno nuevo...\n");
        save_usb_snapshot(path, snapshot_file);
        return;
    }

    struct stat_snapshot old_files[1000];
    int found[1000] = {0};
    int old_count = 0;
    char line[1024];

    while (fgets(line, sizeof(line), in)) {
        if (sscanf(line, "%[^|]|%ld", old_files[old_count].path, &old_files[old_count].mtime) == 2) {
            old_count++;
        }
    }

    fclose(in);

    printf("📦 Cambios detectados desde el último escaneo:\n");

    recursive_compare(path, snapshot_file, old_files, old_count, found);

    for (int i = 0; i < old_count; i++) {
        if (!found[i]) {
            printf("❌ Eliminado: %s\n", old_files[i].path);
        }
    }

    save_usb_snapshot(path, snapshot_file);
}
// Función interna que escribe todos los archivos en 'out'
static void write_snapshot(const char *path, FILE *out) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    if (!dir) {
        perror("No se puede abrir el directorio para snapshot");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Recurse into subdirectorios usando el mismo FILE*
                write_snapshot(full_path, out);
            } else {
                fprintf(out, "%s|%ld\n", full_path, st.st_mtime);
            }
        }
    }
    closedir(dir);
}

// Wrapper que abre el fichero una sola vez
void save_usb_snapshot(const char *path, const char *output_file) {
    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("No se pudo abrir el archivo de snapshot");
        return;
    }

    write_snapshot(path, out);
    fclose(out);

    printf("📁 Snapshot guardado en %s\n", output_file);
}


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