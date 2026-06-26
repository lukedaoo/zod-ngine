nob: build.c
	cc $< -o $@

run: nob
	./nob run $(filter-out $@,$(MAKECMDGOALS))

debug: nob
	./nob debug

release: nob
	./nob release

test: nob
	./nob test $(filter-out $@,$(MAKECMDGOALS))

test-asan: nob
	./nob test-asan $(filter-out $@,$(MAKECMDGOALS))

help: nob
	./nob help

clean:
	@if [ -f nob ]; then ./nob clean; fi
	rm -f nob

.PHONY: run debug release test test-asan help clean
