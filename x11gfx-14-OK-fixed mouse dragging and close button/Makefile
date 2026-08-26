.PHONY: all clean

all: game

game: main.c gl2d.h gl3d.h font.h
	gcc main.c -o game -I. -L. -lX11 -lXext -lXfixes -lm -O3

clean:
	rm game
