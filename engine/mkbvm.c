// Copyright (c) 2023 Sam Blenny
// SPDX-License-Identifier: MIT
//
#include <stdio.h>
#include "mkbvm.h"
#include "libmkb/libmkb.h"

int main() {
	size_t code_len = 1;
	u8 code[1] = {0};
	printf("mk_load_rom() = %d\n", mk_load_rom(code, code_len));
	return mk_core_voc[0].value;
}
