CC        = gcc
CFLAGS    = -Wall -g
INC       = -Iinclude

# Archivos fuente
SRC_USB   = src/main.c src/usb_monitor.c
SRC_PROC  = src/process_monitor_main.c src/process_monitor.c
SRC_GUI   = src/gui.c src/usb_monitor.c src/process_monitor.c

# Binarios de salida
OUT_USB   = build/matcom_guard
OUT_PROC  = build/matcom_guard_proc
OUT_GUI   = build/matcom_gui

# Objetivo principal
all: $(OUT_USB) $(OUT_PROC) $(OUT_GUI)

# Reglas individuales
$(OUT_USB): $(SRC_USB)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

$(OUT_PROC): $(SRC_PROC)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

$(OUT_GUI): $(SRC_GUI)
	$(CC) $(CFLAGS) $(INC) `pkg-config --cflags gtk+-3.0` -o $@ $^ `pkg-config --libs gtk+-3.0`

clean:
	rm -f $(OUT_USB) $(OUT_PROC) $(OUT_GUI)
