CC        = gcc
CFLAGS    = -Wall -g $(shell pkg-config --cflags glib-2.0 gtk+-3.0)
INC       = -Iinclude
LIBS_GUI  = $(shell pkg-config --libs gtk+-3.0 glib-2.0) -pthread

# Archivos fuente
SRC_USB        = src/main.c src/usb_monitor.c
SRC_PROC       = src/process_monitor_main.c src/process_monitor.c
SRC_GUI        = src/gui.c src/usb_monitor.c src/process_monitor.c src/scan_ports.c
SRC_AUTO       = src/usb_monitor_auto.c
SRC_PORTS      = src/scan_ports_main.c src/scan_ports.c
SRC_PROC_AUTO  = src/process_monitor_auto.c src/process_monitor.c

# Binarios de salida
OUT_USB        = build/matcom_guard
OUT_PROC       = build/matcom_guard_proc
OUT_GUI        = build/matcom_gui
OUT_AUTO       = build/matcom_guard_auto
OUT_PORTS      = build/matcom_guard_ports
OUT_PROC_AUTO  = build/matcom_guard_proc_auto

# Objetivo principal
all: $(OUT_USB) $(OUT_PROC) $(OUT_GUI) $(OUT_PORTS) $(OUT_AUTO) $(OUT_PROC_AUTO)

# Consola USB
$(OUT_USB): $(SRC_USB)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

# Consola Procesos
$(OUT_PROC): $(SRC_PROC)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

# Interfaz Gráfica GTK
$(OUT_GUI): $(SRC_GUI)
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS_GUI)

# Versión automática USB
$(OUT_AUTO): $(SRC_AUTO)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

# Escaneo de puertos
$(OUT_PORTS): $(SRC_PORTS)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

# Versión automática Procesos
$(OUT_PROC_AUTO): $(SRC_PROC_AUTO)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

clean:
	rm -f $(OUT_USB) $(OUT_PROC) $(OUT_GUI) $(OUT_AUTO) $(OUT_PORTS) $(OUT_PROC_AUTO)
