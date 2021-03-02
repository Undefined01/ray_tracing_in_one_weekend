GCC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra
LDFLAGS ?= -lm

.DEFAULT_GOAL := all

build/%.o: %.c
	mkdir -p $(dir $@)
	${GCC} -c ${CFLAGS} -o $@ $^

build/raytracing: build/main.o
	${GCC} ${LDFLAGS} -o $@ $^

all: build/raytracing

run: build/raytracing
	build/raytracing > build/out.ppm

clean:
	rm -r build
