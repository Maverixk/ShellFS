CC = gcc
CFLAGS = -Wall -g

all: shell.c fs.h
	$(CC) $(CFLAGS) -o shell shell.c fs.c

.PHONY: clean
clean:
	rm -f shell