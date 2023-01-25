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
TEST_IN1=test/read_test_data.txt
TEST_OUT1=test/vm_out.txt
TEST_IN2=test/rom_test.mkb
TEST_OUT2=test/rom_out.txt

all: kernel.rom mkbvm

run: kernel.rom $(AUTOGEN)
	@echo "TODO: mkbvm kernel.rom"

mkbvm: mkbvm.c mkbvm.h $(C_AUTOGEN) $(LIBMKB_C) $(LIBMKB_H)
	$(CC) -std=c99 -Wall -Wno-unused-function -O3 -o $@ $< libmkb/libmkb.c

selfhost: kernel.rom mkbvm kernel.mkb self_host.mkb $(AUTOGEN)
	@echo "TODO: echo 'load self_host.mkb' | ./mkbvm kernel.rom"
	@echo "TODO: diff self_hosted.rom kernel.rom"

kernel.rom: kernel.mkb bootstrap.py mkbvm $(AUTOGEN)
	@python3 bootstrap.py

codegen: codegen.py $(AUTOGEN)

mkb_autogen.mkb: codegen.py
	@python3 codegen.py

libmkb/autogen.c: codegen.py
	@python3 codegen.py

libmkb/autogen.h: codegen.py
	@python3 codegen.py

test: kernel.rom $(AUTOGEN)
	@echo "TODO: cat $(TEST_IN2) | ./mkbvm kernel.rom | tee $(TEST_OUT2)"

