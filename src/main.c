#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb_monitor.h"

int main() {
    char base_path[256];
    snprintf(base_path, sizeof(base_path), "/media/%s", getenv("USER"));

    printf("=== MatCom Guard: Monitoreo en Tiempo Real de USB ===\n");

    // Mostrar dispositivos conectados
    list_usb_devices(base_path);

    // Pedir nombre del USB
    char usb_name[256];
    printf("\n🔌 Ingresa el nombre de la memoria USB: ");
    scanf("%255s", usb_name);

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, usb_name);

    char snapshot_file[64];
    snprintf(snapshot_file, sizeof(snapshot_file), "/home/%s/.matcomguard_%s_usb_snapshot.txt",getenv("USER"),usb_name);

    printf("\n🕵️ Iniciando vigilancia continua...\n");

    while (1) {
        compare_usb_snapshot(full_path, snapshot_file);
        printf("🔁 Revisión completada. Esperando 5 segundos...\n\n");
        sleep(5);  // Esperar 5 segundos antes de volver a escanear
    }

    return 0;
}

