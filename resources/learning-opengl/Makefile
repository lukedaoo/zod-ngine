NOB = ./nob

.PHONY: all clean help

all: nob
	$(NOB)

clean: nob
	$(NOB) clean

help: nob
	$(NOB) help

nob: build.c
	cc -o nob build.c
