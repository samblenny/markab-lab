# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Makefile for Markab. This is developed and tested on Debian 11 (Bullseye)
# with nasm and clang. For other build environments, YYMV.
#

.POSIX:
.SUFFIXES:
.PHONY: help all clean run dump test
CC=clang
TESTS_OUT=test/make_test_out.txt
TESTS=test/fetch_store.fs test/comments.fs test/dotquote.fs test/stack.fs \
 test/math.fs test/colon_defs.fs test/loops.fs test/fizzbuzz.fs

all: main
clean:
	@rm -f main
	@rm -rf obj

run: main
	@./$<

dump: main
	@objdump --visualize-jumps -d -M intel $<
	@readelf -x .data $<

test: main $(TESTS)
	cat $(TESTS) | ./main | tee $(TESTS_OUT)

main: main.c obj/libmarkab.o
	$(CC) -O2 -Wl,-export-dynamic -std=c99 -Wall -o $@ main.c obj/libmarkab.o

obj/libmarkab.o: libmarkab.nasm screen00.fs generated_data.nasm
	@mkdir -p obj
	nasm -f elf64 -w+all --reproducible -o $@ $<

generated_data.nasm: data_codegen.py
	python3 data_codegen.py > generated_data.nasm
