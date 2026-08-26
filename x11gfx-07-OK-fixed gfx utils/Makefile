.PHONY: all clean

all: test_font

test_font: test_font.c
	gcc test_font.c -o test_font -I. -L. -lX11 -lXext -lXfixes -lm -O3

clean:
	rm test_font


