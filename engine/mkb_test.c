/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifdef PLAN_9
/* Plan 9 inlcudes */
#  include <u.h>              /* u8int, u16int, u32int, ... */
#  include <libc.h>           /* print(), exits(), ... */
#  include <stdio.h>          /* getchar(), putchar() */
#else
/* POSIX includes */
#  include <stdint.h>         /* uint8_t, uint16_t, int32_t, ... */
#  include <stdio.h>          /* printf(), putchar(), ... */
#  include <unistd.h>         /* STDOUT_FILENO */
#endif
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

#ifdef PLAN_9
void main() {
#else
int main() {
#endif
    u8 code[194] = {
        MK_NOP,
        MK_U8, 32, MK_U8, 0, MK_DUMP,
        MK_U8,  1, MK_U8, 0, MK_DUMP,
        MK_U8,  5, MK_U8, 0, MK_DUMP,
        MK_U8,  9, MK_U8, 0, MK_DUMP,
        MK_U8, 13, MK_U8, 0, MK_DUMP,
        MK_U8, 15, MK_U8, 0, MK_DUMP,
        MK_U8, 'E', MK_EMIT,
        MK_U8, 'm', MK_EMIT,
        MK_U8, 'i', MK_EMIT,
        MK_U8, 't', MK_EMIT,
        MK_U8, '\n', MK_EMIT,
        MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_U8, '\n', MK_EMIT, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_U8, '\n', MK_EMIT, MK_DOTSH, MK_U8, '\n', MK_EMIT, MK_DROP,
        MK_DOTSH, MK_U8, '\n', MK_EMIT, MK_DROP,
        MK_DOTSH, MK_U8, '\n', MK_EMIT, MK_DROP,
        MK_DOTSH, MK_U8, '\n', MK_EMIT, MK_DROP,
        MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_U8, '\n', MK_EMIT, MK_U8, 17, MK_I32, 0xef, 0xff, 0xff, 0xff,
        MK_DOT, MK_DOT, MK_U8, '\n', MK_EMIT,
        MK_U8, 17, MK_I32, 0xef, 0xff, 0xff, 0xff,
        MK_HEX, MK_DOT, MK_DOT, MK_DECIMAL, MK_U8, '\n', MK_EMIT,
        MK_DOTSH, MK_U8, '\n', MK_EMIT,
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
/* Output from main() looks like this:
0000  00072007 000d0701 07000d07 0507000d  .. . .... .... ....
0010  07090700 0d070d07 000d070f 07000d0a  .... .... .... ....
0000  00                                   .
0000  00072007 00                          .. . .
0000  00072007 000d0701 07                 .. . .... .
0000  00072007 000d0701 07000d07 05        .. . .... .... .
0000  00072007 000d0701 07000d07 050700    .. . .... .... ...
Emit
 R-Stack is empty
 1f0f
 1f0f 1
 1f0f 1 1abcdef
 1f0f 1 1abcdef e1abcdef

 Stack is empty
 7951
 7951 1
 7951 1 28036591
 7951 1 28036591 -508834321

 1f0f 1 1abcdef e1abcdef
 1f0f 1 1abcdef
 1f0f 1
 1f0f
 Stack is empty

 -17 17
 ffffffef 11
 Stack is empty
mk_load_rom() = 0
*/

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

/* Write byte to stdout */
void mk_host_putchar(u8 data) {
    putchar(data);
}
