#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include "../include/process_monitor.h"

#define MAX_PROC 1024
#define DEFAULT_CPU_THRESHOLD 70.0
#define DEFAULT_MEM_THRESHOLD 50.0

typedef struct {
    int pid;
    char name[64];
    unsigned long utime;
    unsigned long stime;
    long memory_kb;
    double cpu_usage;
} ProcessInfo;

static double cpu_threshold = DEFAULT_CPU_THRESHOLD;
static double mem_threshold = DEFAULT_MEM_THRESHOLD;

unsigned long get_total_cpu_time() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0;
    char buffer[512];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    unsigned long user, nice, system, idle, iowait, irq, softirq;
    sscanf(buffer, "cpu  %lu %lu %lu %lu %lu %lu %lu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);

    return user + nice + system + idle + iowait + irq + softirq;
}

int leer_procesos(ProcessInfo *procs, int *proc_count, unsigned long *total_cpu) {
    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("No se puede abrir /proc");
        return -1;
    }

    *proc_count = 0;
    *total_cpu = get_total_cpu_time();
    struct dirent *entry;

    while ((entry = readdir(proc)) != NULL && *proc_count < MAX_PROC) {
        if (!isdigit(entry->d_name[0])) continue;

        int pid = atoi(entry->d_name);
        char path[256];

        // Leer nombre y memoria
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        FILE *f = fopen(path, "r");
        if (!f) continue;

        char line[256];
        char name[64] = "(desconocido)";
        long mem_kb = -1;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Name: %63s", name) == 1) continue;
            if (sscanf(line, "VmRSS: %ld", &mem_kb) == 1) break;
        }
        fclose(f);

        // Leer tiempo de CPU
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        f = fopen(path, "r");
        if (!f) continue;

        unsigned long utime, stime;
        int skip;
        fscanf(f, "%*d %*s %*c");
        for (int i = 0; i < 11; ++i) fscanf(f, "%d", &skip);
        fscanf(f, "%lu %lu", &utime, &stime);
        fclose(f);

        ProcessInfo *p = &procs[(*proc_count)++];
        p->pid = pid;
        strncpy(p->name, name, sizeof(p->name));
        p->utime = utime;
        p->stime = stime;
        p->memory_kb = mem_kb;
        p->cpu_usage = 0.0; // Se calculará luego
    }

    closedir(proc);
    return 0;
}

void iniciar_monitoreo_procesos() {
    ProcessInfo antes[MAX_PROC], despues[MAX_PROC];
    int count_antes = 0, count_despues = 0;
    unsigned long total_cpu_antes = 0, total_cpu_despues = 0;

    if (leer_procesos(antes, &count_antes, &total_cpu_antes) != 0) return;
    sleep(1);
    if (leer_procesos(despues, &count_despues, &total_cpu_despues) != 0) return;

    unsigned long delta_total_cpu = total_cpu_despues - total_cpu_antes;

    printf("\n🔍 Evaluando procesos por consumo de CPU/RAM...\n");

    for (int i = 0; i < count_despues; ++i) {
        int pid = despues[i].pid;
        for (int j = 0; j < count_antes; ++j) {
            if (antes[j].pid == pid) {
                unsigned long delta_proc_cpu = (despues[i].utime + despues[i].stime)
                                              - (antes[j].utime + antes[j].stime);
                double cpu_percent = 100.0 * delta_proc_cpu / delta_total_cpu;
                despues[i].cpu_usage = cpu_percent;

                int mem_alert = despues[i].memory_kb >= 0 && (despues[i].memory_kb / 1024.0) > mem_threshold;
                int cpu_alert = cpu_percent > cpu_threshold;

                if (cpu_alert || mem_alert) {
                    printf("⚠️  PID %d | %-20s | CPU: %5.1f%% | Mem: %ld KB\n",
                           pid, despues[i].name, cpu_percent, despues[i].memory_kb);
                }
                break;
            }
        }
    }
}
void iniciar_monitoreo_procesos_externo(double cpu, double mem)
{
    cpu_threshold = cpu;
    mem_threshold = mem;
    iniciar_monitoreo_procesos();
}