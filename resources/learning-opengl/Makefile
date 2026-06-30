CC      = cc
CFLAGS  = -std=c23 -Wall -Wextra -I../../lib/glad/include
LDFLAGS = -lglfw -ldl
GLAD    = ../../lib/glad/src/gl.c
SRCS    = $(wildcard src/*.c)
BINS    = $(patsubst src/%.c,bin/%,$(SRCS))

.PHONY: all clean

all: $(BINS)

bin/%: src/%.c | bin
	$(CC) $(CFLAGS) -o $@ $< $(GLAD) $(LDFLAGS)

bin:
	mkdir -p bin

clean:
	rm -rf bin
