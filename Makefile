CC = gcc
CFLAGS = -Wall -g
SRC = src/main.c src/usb_monitor.c
INC = -Iinclude
OUT = build/matcom_guard

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(INC) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)

