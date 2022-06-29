# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
.POSIX:
.SUFFIXES:
.PHONY: all run test clean
AUTOGEN=opcodes.mkb mem_map.mkb opcodes.py mem_map.py core_voc.py
TEST_IN1=test/read_test_data.txt
TEST_OUT1=test/vm_out.txt
TEST_IN2=test/rom_test.mkb
TEST_OUT2=test/rom_out.txt

all: hello.rom kernel.rom

run: kernel.rom $(AUTOGEN)
	@python3 markab_vm.py kernel.rom

hello: hello.rom $(AUTOGEN)
	@python3 markab_vm.py hello.rom

kernel.rom: bootstrap.py $(AUTOGEN)
	@python3 bootstrap.py

hello.rom: hello_rom.py $(AUTOGEN)
	@python3 hello_rom.py

opcodes.mkb: codegen.py
	@python3 codegen.py

mem_map.mkb: codegen.py
	@python3 codegen.py

opcodes.py: codegen.py
	@python3 codegen.py

mem_map.py: codegen.py
	@python3 codegen.py

core_voc.py: codegen.py
	@python3 codegen.py

test: hello.rom kernel.rom $(AUTOGEN)
	@cat $(TEST_IN1) | PYTHONPATH=. python3 test/vm.py | tee $(TEST_OUT1)
	@echo
	@echo "=== rom tests: ==="
	@cat $(TEST_IN2) | python3 markab_vm.py kernel.rom | tee $(TEST_OUT2)

clean:
	@rm -rf __pycache__
