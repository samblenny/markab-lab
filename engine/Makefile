# Copyright (c) 2023 Sam Blenny
# SPDX-License-Identifier: MIT
#
.POSIX:
.SUFFIXES:
.PHONY: wasm

CC=clang
CFLAGS=-ansi -Wall -O3

# CAUTION! Some clang builds (e.g. macOS cli tools) don't support the wasm32
# target. Also LLVM version should be 11+. Check clang suitability like this:
#    $ clang --version
#    Debian clang version 11.0.1-2
#    ...
#    $ clang --print-targets | grep wasm
#        wasm32     - WebAssembly 32-bit
#        wasm64     - WebAssembly 64-bit
#
WASM_C=-ansi -Wall --target=wasm32 -nostdlib -DWASM_MEMCPY
WASM_LD=-Wl,--no-entry -Wl,--export-dynamic -Wl,--allow-undefined -O3 -flto \
 -Wl,--strip-all
WASM_OUT=www/markab-engine.wasm
wasm: mkb_wasm.c Makefile
	clang $(WASM_C) $(WASM_LD) -o $(WASM_OUT) mkb_wasm.c

