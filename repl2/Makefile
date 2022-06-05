# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
.POSIX:
.SUFFIXES:
.PHONY: all run test

all: kernel.bin

run: kernel.bin
	@python3 markab_vm.py

kernel.bin: bootstrap.py
	@python3 bootstrap.py

test:
	@echo 'TODO: write tests'
