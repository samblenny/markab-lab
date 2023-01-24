// Copyright (c) 2023 Sam Blenny
// SPDX-License-Identifier: MIT
//
#include <stdio.h>
#include "mkbvm.h"
#include "mkb_autogen.h"

int main() {
	printf("%s\n", mk_opcodes[3]);
	return mk_core_voc[0].value;
}
