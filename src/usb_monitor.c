#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_FILES 2000

struct baseline_entry {
    char path[1024];
    long size;
    char perms[10];
    long mtime;
    char owner[64];
    char hash[256];
};

struct scanned_file {
    char path[1024];
    char hash[256];
};

void compute_sha256(const char *filepath, char *output, size_t maxlen) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "sha256sum \"%s\" 2>/dev/null", filepath);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        fgets(output, maxlen, fp);
        strtok(output, " ");
        pclose(fp);
    } else {
        strcpy(output, "error");
    }
}

int find_in_baseline(struct baseline_entry *baseline, int count, const char *path) {
    for (int i = 0; i < count; i++) {
        if (strcmp(baseline[i].path, path) == 0) return i;
    }
    return -1;
}

void check_recursive(const char *path,
                     struct baseline_entry *baseline,
                     int baseline_count,
                     int *total_checked,
                     int *suspicious_count,
                     int *found,
                     struct scanned_file *scanned,
                     int *scanned_count) {
    DIR *d = opendir(path);
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (entry->d_name[0] == '.') continue; // ignorar carpetas ocultas

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            check_recursive(full_path, baseline, baseline_count, total_checked, suspicious_count, found, scanned, scanned_count);
        } else {
            (*total_checked)++;

            char current_perm[10];
            snprintf(current_perm, sizeof(current_perm), "%o", st.st_mode & 0777);

            struct passwd *pw = getpwuid(st.st_uid);
            struct group  *gr = getgrgid(st.st_gid);
            const char *owner = pw ? pw->pw_name : "unknown";
            const char *group = gr ? gr->gr_name : "unknown";

            char owner_str[64];
            snprintf(owner_str, sizeof(owner_str), "%s:%s", owner, group);

            char hash[256];
            compute_sha256(full_path, hash, sizeof(hash));

            // Guardar en lista de archivos escaneados
            if (*scanned_count < MAX_FILES) {
                strcpy(scanned[*scanned_count].path, full_path);
                strcpy(scanned[*scanned_count].hash, hash);
                (*scanned_count)++;
            }

            int index = find_in_baseline(baseline, baseline_count, full_path);
            if (index >= 0) {
                found[index] = 1;
            }
            if (index == -1) {
                printf("❗ Archivo nuevo sospechoso: %s\n", full_path);
                (*suspicious_count)++;
            } else {
                struct baseline_entry *b = &baseline[index];

                if (strcmp(b->hash, hash) != 0) {
                    printf("📝 Contenido modificado: %s\n", full_path);
                    (*suspicious_count)++;
                }

                if (b->size != st.st_size && labs(b->size - st.st_size) > b->size * 0.5) {
                    printf("⚠️ Cambio de tamaño inusual: %s (%ld → %ld)\n", full_path, b->size, st.st_size);
                    (*suspicious_count)++;
                }

                if (strcmp(b->perms, current_perm) != 0) {
                    if (strcmp(current_perm, "777") == 0)
                        printf("⚠️ Permiso 777 sospechoso: %s\n", full_path);
                    else
                        printf("⚠️ Permisos modificados: %s (%s → %s)\n", full_path, b->perms, current_perm);
                    (*suspicious_count)++;
                }

                if (strcmp(b->owner, owner_str) != 0) {
                    printf("⚠️ Propietario cambiado: %s (%s → %s)\n", full_path, b->owner, owner_str);
                    (*suspicious_count)++;
                }

                if (b->mtime != st.st_mtime) {
                    printf("⌛ Timestamp modificado: %s\n", full_path);
                    (*suspicious_count)++;
                }
            }
        }
    }
    closedir(d);
}

int load_baseline(const char *file, struct baseline_entry *entries) {
    FILE *f = fopen(file, "r");
    if (!f) return -1;

    int count = 0;
    while (fscanf(f, "%[^|]|%ld|%[^|]|%ld|%[^|]|%s\n",
                  entries[count].path,
                  &entries[count].size,
                  entries[count].perms,
                  &entries[count].mtime,
                  entries[count].owner,
                  entries[count].hash) == 6) {
        count++;
    }
    fclose(f);
    return count;
}

void save_baseline_recursive(const char *path, FILE *out) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (entry->d_name[0] == '.') continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            save_baseline_recursive(full_path, out);
        } else {
            char perm[10];
            snprintf(perm, sizeof(perm), "%o", st.st_mode & 0777);

            struct passwd *pw = getpwuid(st.st_uid);
            struct group  *gr = getgrgid(st.st_gid);
            const char *owner = pw ? pw->pw_name : "unknown";
            const char *group = gr ? gr->gr_name : "unknown";

            char hash[256];
            compute_sha256(full_path, hash, sizeof(hash));

            fprintf(out, "%s|%ld|%s|%ld|%s:%s|%s\n",
                    full_path,
                    (long)st.st_size,
                    perm,
                    (long)st.st_mtime,
                    owner,
                    group,
                    hash);
        }
    }
    closedir(dir);
}

void save_usb_security_baseline(const char *usb_path, const char *baseline_file) {
    FILE *out = fopen(baseline_file, "w");
    if (!out) {
        perror("No se pudo guardar el baseline");
        return;
    }
    save_baseline_recursive(usb_path, out);
    fclose(out);
    printf("\U0001F6E1 Baseline de seguridad actualizado en %s\n", baseline_file);
}

void compare_usb_security_baseline(const char *usb_path, const char *baseline_file, double alert_threshold_percent) {
    struct baseline_entry baseline[MAX_FILES];
    struct scanned_file scanned[MAX_FILES];
    int baseline_count = load_baseline(baseline_file, baseline);
    if (baseline_count < 0) {
        printf("⚠️ No se pudo leer el baseline. Crea uno primero.\n");
        return;
    }

    int total_checked = 0, suspicious_count = 0;
    int found[MAX_FILES] = {0};
    int scanned_count = 0;

    check_recursive(usb_path, baseline, baseline_count, &total_checked, &suspicious_count,
                    found, scanned, &scanned_count);

    for (int i = 0; i < baseline_count; ++i) {
        if (!found[i]) {
            printf("❌ Eliminado: %s\n", baseline[i].path);
        }
    }

    // Detectar archivos replicados
    for (int i = 0; i < scanned_count; ++i) {
        for (int j = i + 1; j < scanned_count; ++j) {
            if (strcmp(scanned[i].hash, scanned[j].hash) == 0 &&
                strcmp(scanned[i].path, scanned[j].path) != 0) {
                printf("⚠️ Posible réplica: %s ≈ %s\n", scanned[i].path, scanned[j].path);
            }
        }
    }

    double percent = (total_checked == 0) ? 0.0 : ((double)suspicious_count / total_checked) * 100;
    if (percent >= alert_threshold_percent) {
        printf("🚨 ALERTA: Cambios sospechosos en %.1f%% de los archivos.\n", percent);
    } else {
        printf("✅ Análisis completo. Cambios sospechosos: %.1f%%\n", percent);
    }

    save_usb_security_baseline(usb_path, baseline_file);
}
