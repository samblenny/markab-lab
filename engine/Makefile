# Copyright (c) 2023 Sam Blenny
# SPDX-License-Identifier: MIT
#
.SUFFIXES:
.PHONY: all run test clean selfhost
AUTOGEN=opcodes.mkb mem_map.mkb opcodes.h mem_map.h core_voc.h
TEST_IN1=test/read_test_data.txt
TEST_OUT1=test/vm_out.txt
TEST_IN2=test/rom_test.mkb
TEST_OUT2=test/rom_out.txt

all: kernel.rom mkbvm

run: kernel.rom $(AUTOGEN)
	@echo "TODO: mkbvm kernel.rom"

mkbvm: mkbvm.c $(AUTOGEN)
	@echo "TODO: build markab vm"

selfhost: kernel.rom mkbvm kernel.mkb self_host.mkb $(AUTOGEN)
	@echo "TODO: echo 'load self_host.mkb' | ./mkbvm kernel.rom"
	@echo "TODO: diff self_hosted.rom kernel.rom"

kernel.rom: kernel.mkb bootstrap mkbvm $(AUTOGEN)
	@echo "TODO: ./bootstrap"

opcodes.mkb: codegen
	@echo "TODO: ./codegen"

mem_map.mkb: codegen
	@echo "TODO: ./codegen"

opcodes.h: codegen
	@echo "TODO: ./codegen"

mem_map.h: codegen
	@echo "TODO: ./codegen"

core_voc.h: codegen
	@echo "TODO: ./codegen"

test: kernel.rom $(AUTOGEN)
	@echo "TODO: cat $(TEST_IN2) | ./mkbvm kernel.rom | tee $(TEST_OUT2)"
