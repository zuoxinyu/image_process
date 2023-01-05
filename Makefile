CC = clang
INCDIR = src
CFLAGS = -Wall -g -I$(INCDIR) -DUSE_FFTW3
LIBS = -lm -lSDL2 $(shell pkg-config fftw3f --libs) $(shell pkg-config fftw3 --libs)
LIBSRC=$(wildcard src/*.c)
CMDDIR= .

.PHONY: all run clean

all: main dctgen

main: $(LIBSRC) $(CMDDIR)/main.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

dctgen: $(LIBSRC) $(CMDDIR)/dctgen.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

run: $(LIBSRC) main
	./main Lenna.ppm

clean:
	rm -f main dcttran
