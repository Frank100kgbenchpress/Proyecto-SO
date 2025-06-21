#include <stdio.h>
#include <unistd.h>
#include "process_monitor.h"

int main() {
    while (1) {
        iniciar_monitoreo_procesos_externo(70.0, 50.0);  // Usa los umbrales deseados
        fflush(stdout);
        sleep(60);
    }
    return 0;
}
