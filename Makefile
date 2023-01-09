CC = clang
CFLAG_MSAN = 
#CFLAG_MSAN = -fsanitize=address -fno-omit-frame-pointer
LIBDIR = src
LIBSRC=$(wildcard $(LIBDIR)/*.c)
GUISRC=window.c
CMDDIR= .

MAIN_CMD = main
IMGN_CMD = imgn

CFLAGS = -Wall -g -O0 -I$(LIBDIR) -DUSE_FFTW3 $(CFLAG_MSAN)
LIBS = -lm -lSDL2 $(shell pkg-config fftw3f --libs) $(shell pkg-config fftw3 --libs)

.PHONY: all run clean

all: $(MAIN_CMD) $(IMGN_CMD)

$(MAIN_CMD): $(LIBSRC) $(GUISRC) $(CMDDIR)/$(MAIN_CMD).c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(IMGN_CMD): $(LIBSRC) $(CMDDIR)/$(IMGN_CMD).c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

run: $(LIBSRC) $(MAIN_CMD)
	$(CMDDIR)/$(MAIN_CMD) Lenna.ppm

clean:
	rm -f $(MAIN_CMD) $(IMGN_CMD)
