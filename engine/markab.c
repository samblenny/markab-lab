/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Markab example CLI front-end for POSIX and Plan 9
 */
#ifdef PLAN_9
/* Plan 9 inlcudes */
#  include <u.h>              /* u8int, u16int, u32int, ... */
#  include <libc.h>           /* print(), exits(), ... */
#  include <stdio.h>          /* getchar(), putchar() */
#else
/* POSIX includes */
#  include <stdint.h>         /* uint8_t, uint16_t, int32_t, ... */
#  include <stdio.h>          /* printf(), getchar(), putchar(), ... */
#  include <unistd.h>         /* STDOUT_FILENO */
#endif
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

#ifdef PLAN_9
void main() {
#else
int main() {
#endif
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
#ifdef PLAN_9
    print("mk_load_rom() = %d\n", mk_load_rom(code, rom_size));
    exits(0);
#else
    printf("mk_load_rom() = %d\n", mk_load_rom(code, rom_size));
    return 0;
#endif
}

/* Write an error code to stderr */
void mk_host_log_error(u8 error_code) {
#ifdef PLAN_9
    print("mk_host_log_code(%d)\n", error_code);
#else
    printf("mk_host_log_code(%d)\n", error_code);
#endif
}

/* Write length bytes from byte buffer buf to stdout */
void mk_host_stdout_write(const void * buf, int length) {
#ifdef PLAN_9
    write(1 /* STDOUT */, buf, length);
#else
    write(STDOUT_FILENO, buf, length);
#endif
}

/* Read byte from stdin to *data, returning 0 for success or 1 for EOF */
u8 mk_host_getchar(u8 * data) {
    int c = getchar();
    if(c != EOF) {
        *data = (u8) c;
        return 0;
    }
    return 1;
}

/* Write byte to stdout */
void mk_host_putchar(u8 data) {
    putchar(data);
}
