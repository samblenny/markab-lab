# Copyright (c) 2023 Sam Blenny
# SPDX-License-Identifier: MIT
#
.POSIX:
.SUFFIXES:
.PHONY: run test clean codegen wasm

CC=clang
CFLAGS=-ansi -Wall -O3

AUTOGEN=libmkb/autogen.h libmkb/autogen.c
CLEAN_RM=markab mkb_test
LIBMKB_C=libmkb/libmkb.c libmkb/op.c libmkb/vm.c libmkb/fmt.c libmkb/comp.c
LIBMKB_H=libmkb/libmkb.h libmkb/op.h libmkb/vm.h libmkb/fmt.h libmkb/comp.h

markab: markab.c $(AUTOGEN) $(LIBMKB_C) $(LIBMKB_H) Makefile
	$(CC) $(CFLAGS) -o markab markab.c libmkb/libmkb.c

mkb_test: mkb_test.c $(AUTOGEN) $(LIBMKB_C) $(LIBMKB_H) Makefile
	$(CC) $(CFLAGS) -o mkb_test mkb_test.c libmkb/libmkb.c

run: markab
	./markab

test: mkb_test
	./mkb_test

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
wasm: mkb_wasm.c $(AUTOGEN) $(LIBMKB_C) $(LIBMKB_H) Makefile
	clang $(WASM_C) $(WASM_LD) -o $(WASM_OUT) mkb_wasm.c

clean:
	@rm -f $(CLEAN_RM)

codegen:
	@python3 codegen.py

libmkb/autogen.c: codegen.py
	@python3 codegen.py

libmkb/autogen.h: codegen.py
	@python3 codegen.py

