/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Markab example CLI front-end
 */
#include <stdint.h>         /* uint8_t, uint16_t, int32_t, ... */
#include <stdio.h>          /* printf(), getchar(), putchar(), ... */
#include <unistd.h>         /* STDOUT_FILENO */
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

int main() {
    u8 code[100] = {
        MK_U8, 'h', MK_EMIT,
        MK_U8, 'e', MK_EMIT,
        MK_U8, 'l', MK_EMIT,
        MK_U8, 'l', MK_EMIT,
        MK_U8, 'o', MK_EMIT,
        MK_U8, ',', MK_EMIT,
        MK_U8, ' ', MK_EMIT,
        MK_U8, 'w', MK_EMIT,
        MK_U8, 'o', MK_EMIT,
        MK_U8, 'r', MK_EMIT,
        MK_U8, 'l', MK_EMIT,
        MK_U8, 'd', MK_EMIT,
        MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    int rom_size = sizeof(code) / sizeof(code[0]);
    printf("mk_load_rom() = %d\n", mk_load_rom(code, rom_size));
    return 0;
}

/* Log an error code to stdout */
void mk_host_log_error(u8 error_code) {
    printf("mk_host_log_error(%d)\n", error_code);
}

/* Write length bytes from byte buffer buf to stdout */
void mk_host_stdout_write(const void * buf, int length) {
    write(1 /* STDOUT */, buf, length);
}

/* Format an integer to stdout */
void mk_host_stdout_fmt_int(int n) {
    printf("%d", n);
}

/* Write byte to stdout */
void mk_host_putchar(u8 data) {
    putchar(data);
}
