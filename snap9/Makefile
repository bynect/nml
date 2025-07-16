CC=gcc
CFLAGS=-O2 -g -Wall -Wextra
CDEFS=-DMAC_PREFETCH
LFLAGS=

INC=$(wildcard *.h)
SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))

nml: $(OBJ)
	$(CC) $(LFLAGS) -o $@ $^

%.o: %.c $(INC)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJ) nml
