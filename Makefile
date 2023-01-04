LIBS  = -lm -lSDL2 $(shell pkg-config fftw3f --libs) $(shell pkg-config fftw3 --libs)
CFLAGS = -Wall
SRC=$(wildcard src/*.c)

ui: $(SRC)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)
