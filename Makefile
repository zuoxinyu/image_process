CC = clang
INCDIR = src
#CFLAG_MSAN = 
CFLAG_MSAN = -fsanitize=address -fno-omit-frame-pointer
CFLAGS = -Wall -g -I$(INCDIR) -DUSE_FFTW3 $(CFLAG_MSAN)
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
