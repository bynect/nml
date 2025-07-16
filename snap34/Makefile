CC=gcc
CFLAGS=-O2 -flto -g -Wall -Wextra
CDEFS=-DMAC_PREFETCH
LDFLAGS=-flto

INC=$(wildcard *.h)
SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))

nml: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c $(INC)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJ) nml
