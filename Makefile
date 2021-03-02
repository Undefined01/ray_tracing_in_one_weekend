GCC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -march=native -O3
LDFLAGS ?= -lm

.DEFAULT_GOAL := all

build/%.o: %.c
	mkdir -p $(dir $@)
	${GCC} -c ${CFLAGS} -o $@ $^

build/raytracing: build/main.o
	${GCC} ${LDFLAGS} -o $@ $^

all: build/raytracing

run: build/raytracing
	bash -c 'time build/raytracing > build/out.ppm'

clean:
	rm -r build
