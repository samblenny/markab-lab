/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"

int main() {
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
    printf("mk_load_rom() = %d\n", mk_load_rom(code, 194));
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
