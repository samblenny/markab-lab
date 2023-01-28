/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * This is a front-end for build testing on Plan 9.
 */
#include <u.h>
#include <libc.h>
#include "libmkb/libmkb.h"

#define CODE_LEN 1
void main() {
    u8 code[CODE_LEN] = {0};
    print("mk_load_rom() = %d\n", mk_load_rom(code, CODE_LEN));
    exits(0);
}
