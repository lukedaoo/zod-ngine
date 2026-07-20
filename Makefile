nob: build.c
	cc $< -o $@

run: nob
	./nob run $(filter-out $@,$(MAKECMDGOALS))

build-debug: nob
	./nob build-debug $(filter-out $@,$(MAKECMDGOALS))

build-release: nob
	./nob build-release $(filter-out $@,$(MAKECMDGOALS))

test: nob
	./nob test $(filter-out $@,$(MAKECMDGOALS))

test-asan: nob
	./nob test-asan $(filter-out $@,$(MAKECMDGOALS))

help: nob
	./nob help

# info(makefile-working):configuration system
working: nob
	./nob build-debug
	./engine_run --log-level trace

clean:
	@if [ -f nob ]; then ./nob clean; fi
	rm -f nob

.PHONY: run build-debug build-release test test-asan help working clean
