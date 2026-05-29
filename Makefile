CC65_DIR = ../cc65
CC65 = $(CC65_DIR)/bin/cl65

CFLAGS = -O -t nes

all: game.nes

game.nes: main.c
	$(CC65) $(CFLAGS) -o game.nes main.c

clean:
	del /Q game.nes main.o
