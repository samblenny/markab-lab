/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * This is a front-end for build testing on Plan 9.
 */
#include <u.h>
#include <libc.h>
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

void main() {
    u8 code[32] = {
        MK_NOP,
        MK_U8, 32, MK_U8, 0, MK_IODUMP,
        MK_U8,  1, MK_U8, 0, MK_IODUMP,
        MK_U8,  5, MK_U8, 0, MK_IODUMP,
        MK_U8,  9, MK_U8, 0, MK_IODUMP,
        MK_U8, 13, MK_U8, 0, MK_IODUMP,
        MK_U8, 15, MK_U8, 0, MK_IODUMP,
        MK_HALT,
    };
    print("mk_load_rom() = %d\n", mk_load_rom(code, 32));
    exits(0);
}
/* Output from main() looks like this:
0000  00072007 000d0701 07000d07 0507000d  .. . .... .... ....
0010  07090700 0d070d07 000d070f 07000d0a  .... .... .... ....
0000  00                                   .
0000  00072007 00                          .. . .
0000  00072007 000d0701 07                 .. . .... .
0000  00072007 000d0701 07000d07 05        .. . .... .... .
0000  00072007 000d0701 07000d07 050700    .. . .... .... ...
mk_load_rom() = 0
*/

/* Write an error code to stderr */
void mk_host_log_error(u8 error_code) {
    print("mk_host_log_code(%d)\n", error_code);
}

/* Write length bytes from byte buffer buf to stdout */
void mk_host_stdout_write(const void * buf, int length) {
    write(1 /* STDOUT */, buf, length);
}
