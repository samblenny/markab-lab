# Copyright (c) 2023 Sam Blenny
# SPDX-License-Identifier: MIT
#
.SUFFIXES:
.PHONY: all run test clean selfhost codegen
C_AUTOGEN=libmkb/autogen.h libmkb/autogen.c
AUTOGEN=mkb_autogen.mkb $(C_AUTOGEN)
LIBMKB_C=libmkb/libmkb.c libmkb/op.c libmkb/vm.c
LIBMKB_H=libmkb/libmkb.h libmkb/op.h libmkb/vm.h
CC=clang
CFLAGS=-ansi -Wall -O3
TEST_IN=test/rom_test.mkb
TEST_OUT=test/rom_out.txt

all: mkbvm

run: kernel.mkb mkbvm $(AUTOGEN)
	mkbvm kernel.mkb

mkbvm: mkbvm.c $(C_AUTOGEN) $(LIBMKB_C) $(LIBMKB_H) Makefile
	$(CC) $(CFLAGS) -o mkbvm mkbvm.c libmkb/libmkb.c

selfhost: kernel.rom mkbvm kernel.mkb self_host.mkb $(AUTOGEN)
	@echo "TODO: echo 'load self_host.mkb' | ./mkbvm kernel.rom"
	@echo "TODO: diff self_hosted.rom kernel.rom"

codegen:
	@python3 codegen.py

mkb_autogen.mkb: codegen.py
	@python3 codegen.py

libmkb/autogen.c: codegen.py
	@python3 codegen.py

libmkb/autogen.h: codegen.py
	@python3 codegen.py

test: kernel.mkb mkbvm $(AUTOGEN) Makefile
	@echo "TODO: cat $(TEST_IN) | ./mkbvm kernel.rom | tee $(TEST_OUT)"

clean:
	@rm -f mkbvm
