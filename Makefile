CC        = gcc
CFLAGS    = -Wall -g
INC       = -Iinclude

SRC       = src/main.c src/usb_monitor.c
OUT       = build/matcom_guard

GUI_SRC   = src/gui.c
GUI_BIN   = build/matcom_gui

# Objetivo por defecto: construye ambos binarios
all: $(OUT) $(GUI_BIN)

# Regla para el binario de consola
$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

# Regla para la interfaz gráfica (GTK)
$(GUI_BIN): $(GUI_SRC)
	$(CC) $(CFLAGS) $(INC) `pkg-config --cflags gtk+-3.0` $< -o $@ `pkg-config --libs gtk+-3.0`

# Limpia ambos binarios
clean:
	rm -f $(OUT) $(GUI_BIN)

