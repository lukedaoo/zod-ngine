all: run

run: | nob
	./nob run

test: | nob
	./nob test

clean:
	@if [ -f nob ]; then ./nob clean; fi
	rm -f nob

nob: build.c
	cc build.c -o nob

.PHONY: all run test clean nob
