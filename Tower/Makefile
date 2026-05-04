CC = gcc
CFLAGS = -Wall -O2

.PHONY: all gtk terminal clean

all: gtk terminal

gtk: hanoi_gtk-1.c
	gcc -o hanoi_gtk.exe hanoi_gtk-1.c $(shell pkg-config --cflags --libs gtk+-3.0) -lm -Wall

terminal: hanoi-2.c
	gcc -o hanoi.exe hanoi-2.c -Wall

clean:
	rm -f hanoi_gtk.exe hanoi.exe