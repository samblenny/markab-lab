# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
.POSIX:
.SUFFIXES:
.PHONY: all run test clean
AUTOGEN=tokens.fs mem_map.fs tokens.py mem_map.py

all: kernel.bin

run: kernel.bin $(AUTOGEN)
	@python3 markab_vm.py

kernel.bin: bootstrap.py $(AUTOGEN)
	@python3 bootstrap.py

tokens.fs: codegen.py
	@python3 codegen.py

mem_map.fs: codegen.py
	@python3 codegen.py

tokens.py: codegen.py
	@python3 codegen.py

mem_map.py: codegen.py
	@python3 codegen.py

test: kernel.bin
	@PYTHONPATH=. python3 test/vm.py | tee test/vm_out.txt

clean:
	@rm -rf __pycache__
