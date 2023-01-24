# Copyright (c) 2023 Sam Blenny
# SPDX-License-Identifier: MIT
#
.SUFFIXES:
.PHONY: all run test clean selfhost codegen
C_AUTOGEN=mkb_autogen.h mkb_autogen.c
AUTOGEN=mkb_autogen.mkb mkb_autogen.py $(C_AUTOGEN)
CC=clang
TEST_IN1=test/read_test_data.txt
TEST_OUT1=test/vm_out.txt
TEST_IN2=test/rom_test.mkb
TEST_OUT2=test/rom_out.txt

all: kernel.rom mkbvm

run: kernel.rom $(AUTOGEN)
	@echo "TODO: mkbvm kernel.rom"

mkbvm: mkbvm.c mkbvm.h $(C_AUTOGEN)
	$(CC) -std=c99 -Wall -O3 -o $@ $< mkb_autogen.c

selfhost: kernel.rom mkbvm kernel.mkb self_host.mkb $(AUTOGEN)
	@echo "TODO: echo 'load self_host.mkb' | ./mkbvm kernel.rom"
	@echo "TODO: diff self_hosted.rom kernel.rom"

kernel.rom: kernel.mkb bootstrap.py mkbvm $(AUTOGEN)
	@python3 bootstrap.py

codegen: codegen.py $(AUTOGEN)

mkb_autogen.mkb: codegen.py
	@python3 codegen.py

mkb_autogen.py: codegen.py
	@python3 codegen.py

mkb_autogen.h: codegen.py
	@python3 codegen.py

test: kernel.rom $(AUTOGEN)
	@echo "TODO: cat $(TEST_IN2) | ./mkbvm kernel.rom | tee $(TEST_OUT2)"

