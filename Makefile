#!/usr/bin/make
.SUFFIXES:
.PHONY: all run pack clean

TAR = ult
PCK = lab-4.zip
SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)
CFLAGS = -std=gnu11 -c -ggdb -O0 -Wall -Werror -Wno-unused-value -Wno-deprecated

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

ult: $(OBJ)
	$(CC) $^ -o $@

all: $(TAR)

run: all
	./$(TAR)

pack: clean
	zip $(PCK) *.c *.h Makefile -x "./.*"

clean:
	$(RM) $(RMFILES) $(OBJ) $(TAR)
