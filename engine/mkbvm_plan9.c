/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * This is a front-end for build testing on Plan 9.
 */
#include <u.h>
#include <libc.h>
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

#define CODE_LEN 1
void main() {
    u8 code[2] = {MK_NOP, MK_HALT};
    print("mk_load_rom() = %d\n", mk_load_rom(code, CODE_LEN));
    exits(0);
}

void mk_host_log_error(u8 error_code) {
    print("mk_host_log_code(%d)\n", error_code);
}
