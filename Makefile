CC=gcc
CFLAGS=-Wall -Wextra -Wvla -Wpedantic

game: main.c
	$(CC) $(CFLAGS) -o game main.c

run:
	./game
	magick convert gaem.ppm gaem.png

clean:
	rm -f game *.o
