#include <stdio.h>
#include "usb_monitor.h"

int main() {
    printf("=== MatCom Guard: Prueba de Dispositivos USB ===\n");
    list_usb_devices("/media"); // O usa "/media/$USER" si tu distro lo requiere

    return 0;
}
