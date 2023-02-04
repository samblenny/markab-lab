# Copyright (c) 2023 Sam Blenny
# SPDX-License-Identifier: MIT
#
.POSIX:
.SUFFIXES:
.PHONY: run test clean codegen

CC=clang
CFLAGS=-ansi -Wall -O3

AUTOGEN=libmkb/autogen.h libmkb/autogen.c
CLEAN_RM=markab mkb_test
LIBMKB_C=libmkb/libmkb.c libmkb/op.c libmkb/vm.c libmkb/fmt.c libmkb/compile.c
LIBMKB_H=libmkb/libmkb.h libmkb/op.h libmkb/vm.h libmkb/fmt.h libmkb/compile.h

markab: markab.c $(AUTOGEN) $(LIBMKB_C) $(LIBMKB_H) Makefile
	$(CC) $(CFLAGS) -o markab markab.c libmkb/libmkb.c

mkb_test: mkb_test.c $(AUTOGEN) $(LIBMKB_C) $(LIBMKB_H) Makefile
	$(CC) $(CFLAGS) -o mkb_test mkb_test.c libmkb/libmkb.c

run: markab
	./markab

test: mkb_test
	./mkb_test

clean:
	@rm -f $(CLEAN_RM)

codegen:
	@python3 codegen.py

libmkb/autogen.c: codegen.py
	@python3 codegen.py

libmkb/autogen.h: codegen.py
	@python3 codegen.py

