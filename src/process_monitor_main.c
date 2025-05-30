#include <stdio.h>
#include <stdlib.h>
#include "process_monitor.h"

double cpu_threshold = 70.0;
double mem_threshold = 50.0;

void iniciar_monitoreo_procesos_externo(double cpu, double mem);

int main(int argc, char *argv[]) {
    if (argc == 3) {
        cpu_threshold = atof(argv[1]);
        mem_threshold = atof(argv[2]);
    }

    iniciar_monitoreo_procesos_externo(cpu_threshold, mem_threshold);
    return 0;
}
