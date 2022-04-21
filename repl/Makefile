# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Makefile for Markab. This is developed and tested on Debian 11 (Bullseye)
# with nasm and clang. For other build environments, YYMV.
#

.POSIX:
.SUFFIXES:
.PHONY: help all clean run dump
CC=clang

all: main
clean:
	@rm -f main
	@rm -rf obj

run: main
	@./$<

dump: main
	@objdump --visualize-jumps -d -M intel $<
	@readelf -x .data $<

main: main.c obj/libmarkab.o
	$(CC) -O2 -Wl,-export-dynamic -std=c99 -Wall -o $@ main.c obj/libmarkab.o

# Enable `make obj/foo.o` to create object file `obj/foo.o` from `foo.nasm`
obj/%.o: %.nasm
	@mkdir -p obj
	@nasm -f elf64 -w+all --reproducible -o $@ $<
