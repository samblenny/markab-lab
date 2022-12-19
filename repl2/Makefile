# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
.SUFFIXES:
.PHONY: all run test clean selfhost
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

selfhost: kernel.rom markab_vm.py kernel.mkb self_host.mkb $(AUTOGEN)
	echo 'load self_host.mkb' | ./markab_vm.py kernel.rom
	hexdump -C self_hosted.rom > self_hosted.rom.hex.txt
	hexdump -C kernel.rom > kernel.rom.hex.txt

kernel.rom: kernel.mkb bootstrap.py markab_vm.py $(AUTOGEN)
	@python3 bootstrap.py

hello.rom: hello_rom.py markab_vm.py $(AUTOGEN)
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

romtest: kernel.rom $(AUTOGEN)
	@cat $(TEST_IN2) | python3 markab_vm.py kernel.rom | tee $(TEST_OUT2)

# The -fopenmp enables OpenMP parallelization, but it does not work with clang
# on macOS (uname -> "Darwin"). So, use a gnu-make conditional to make it work
hash_stats: hash_stats.c
ifeq ($(shell uname),Darwin)
	$(CC) -std=c99 -Wall -O3 -o $@ $<
else
	@OMP_NUM_THREADS=4 $(CC) -fopenmp -std=c99 -Wall -O3 -o $@ $<
endif

clean:
	@rm -rf __pycache__ hash_stats
