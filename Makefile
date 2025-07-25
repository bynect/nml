CFLAGS=-O1 -g3 -Wall -Wextra
LDFLAGS=-Wl,-O1

INC=$(wildcard *.h)
SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
BIN=nmlc

$(BIN): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $^

%.o: %.c $(INC)
	$(CC) -o $@ -c $(CFLAGS) $<

.PHONY: clean
clean:
	rm -f $(OBJ) $(BIN)
