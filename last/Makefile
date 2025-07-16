CC=gcc
CFLAGS=-O2 -g -Wall -Wextra
LDFLAGS=-Wl,-O1
CPPFLAGS=#-DMAC_PREFETCH -DMAC_LIKELINESS

CFLAGS+=-fsanitize=address
LDFLAGS+=-fsanitize=address

INC=$(wildcard *.h)
SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))

nml: $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $^

%.o: %.c $(INC)
	$(CC) -o $@ -c $(CFLAGS) $<

.PHONY: clean
clean:
	rm -f $(OBJ) nml
