# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
.POSIX:
.SUFFIXES:
.PHONY: all run test clean
AUTOGEN=opcodes.mkb mem_map.mkb opcodes.py mem_map.py core_voc.py
TEST_IN=test/read_test_data.txt
TEST_OUT=test/vm_out.txt

all: kernel.bin

run: $(AUTOGEN)
	@python3 bootstrap.py
	@python3 markab_vm.py

kernel.bin: bootstrap.py $(AUTOGEN)
	@python3 bootstrap.py

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

test: kernel.bin
	@cat $(TEST_IN) | PYTHONPATH=. python3 test/vm.py | tee $(TEST_OUT)

clean:
	@rm -rf __pycache__
