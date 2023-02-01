/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

int main() {
    u8 code[47] = {
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
        MK_HALT,
    };
    printf("mk_load_rom() = %d\n", mk_load_rom(code, 47));
    return 0;
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
mk_load_rom() = 0
*/

/* Write an error code to stderr */
void mk_host_log_error(u8 error_code) {
    printf("mk_host_log_code(%d)\n", error_code);
}

/* Write length bytes from byte buffer buf to stdout */
void mk_host_stdout_write(const void * buf, int length) {
    write(STDOUT_FILENO, buf, length);
}

/* Read byte from stdin to *data, returning 0 for success or 1 for EOF */
u8 mk_host_getchar(u8 * data) {
    int c = getchar();
    if(c != EOF) {
        *data = (u8) c;
        return 0;
    }
    /* The getchar() docs say to check feof() and ferror() to learn whether
     * the EOF code indicated a normal end of file or a file IO error. But, for
     * my purposes here, the distinction makes no difference.
     */
    return 1;
}

/* Write byte to stdout */
void mk_host_putchar(u8 data) {
    putchar(data);
    /* TODO: Should I check for EOF? Is it more fun to just ignore it? */
}
