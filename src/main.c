#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb_monitor.h"

int main() {
    char base_path[256];
    snprintf(base_path, sizeof(base_path), "/media/%s", getenv("USER"));

    printf("=== MatCom Guard: Detección de Cambios en USB ===\n");

    // Paso 1: Mostrar los dispositivos conectados
    list_usb_devices(base_path);

    // Paso 2: Pedir el nombre del dispositivo al usuario
    char usb_name[256];
    printf("\n🔌 Ingresa el nombre de la memoria USB: ");
    scanf("%255s", usb_name);

    // Paso 3: Crear ruta completa a la memoria
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, usb_name);

    // Paso 4: Ruta al archivo de snapshot (temporalmente fijo en el proyecto)
    char snapshot_file[64];
    snprintf(snapshot_file, sizeof(snapshot_file), "usb_snapshot.txt");

    // Paso 5: Comparar contra snapshot anterior
    compare_usb_snapshot(full_path, snapshot_file);

    return 0;
}

