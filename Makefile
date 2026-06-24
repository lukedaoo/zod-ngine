nob: build.c
	cc $< -o $@

run: nob
	./nob run

test: nob
	./nob test

clean:
	@if [ -f nob ]; then ./nob clean; fi
	rm -f nob

.PHONY: run test clean
