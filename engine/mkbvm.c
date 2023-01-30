/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdio.h>
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

int main() {
    size_t code_len = 1;
    u8 code[2] = {MK_NOP, MK_HALT};
    printf("mk_load_rom() = %d\n", mk_load_rom(code, code_len));
    return 0;
}

void mk_host_log_error(u8 error_code) {
    printf("mk_host_log_code(%d)\n", error_code);
}
