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
CFLAGS=-std=c99 -Wall
LDFLAGS=-O2 -Wl,-export-dynamic
TESTS_OUT=test/make_test_out.txt
TESTS=test/fetch_store.fs test/comments.fs test/dotquote.fs test/stack.fs \
  test/math.fs test/colon_defs.fs test/loops.fs test/fizzbuzz.fs \
  test/return_stack.fs test/var_const.fs test/control_flow.fs
ASM_OBJS=obj/vm/boolean.o obj/vm/data_stack.o obj/vm/math.o \
  obj/vm/return_stack.o obj/vm/store_fetch.o \
  obj/compiler.o obj/debugging.o obj/errors.o obj/numbers.o obj/strings.o
AUTOGEN_SRC=libmarkab/generated_data.nasm libmarkab/generated_macros.nasm
COMMON_SRC=libmarkab/common_macros.nasm

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

main: main.c obj/libmarkab.o $(ASM_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ main.c obj/libmarkab.o $(ASM_OBJS)

obj/libmarkab.o: libmarkab.nasm $(AUTOGEN_SRC) $(COMMON_SRC) screen00.fs
	@mkdir -p obj/vm
	nasm -f elf64 -w+all --reproducible -o $@ $<

obj/%.o: libmarkab/%.nasm $(AUTOGEN_SRC) $(COMMON_SRC)
	@mkdir -p obj/vm
	@nasm -f elf64 -w+all --reproducible -o $@ $<

libmarkab/generated_data.nasm: data_codegen.py
	python3 data_codegen.py

libmarkab/generated_macros.nasm: data_codegen.py
	python3 data_codegen.py
