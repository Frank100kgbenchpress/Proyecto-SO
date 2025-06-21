#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define CONFIG_USB_PATH_FORMAT "/home/%s/.matcomguard.conf"

double load_alert_threshold() {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_USB_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "r");
    double threshold = 10.0;
    if (f) {
        fscanf(f, "%lf", &threshold);
        fclose(f);
    }
    return threshold;
}

int main() {
    double threshold = load_alert_threshold();
    char base_path[256];
    snprintf(base_path, sizeof(base_path), "/media/%s", getenv("USER"));
    DIR *dir = opendir(base_path);
    if (!dir) {
        printf("⚠️  No se pudo acceder a /media/%s\n", getenv("USER"));
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "./build/matcom_guard \"%s\" %.1f", entry->d_name, threshold);
            FILE *fp = popen(cmd, "r");
            if (fp) {
                char line[1024];
                while (fgets(line, sizeof(line), fp)) {
                    fputs(line, stdout); // redirige al GUI
                }
                pclose(fp);
            }
        }
    }

    closedir(dir);
    return 0;
}
