/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Minimalist DIY string formatting functions.
 */
#ifndef LIBMKB_FMT_C
#define LIBMKB_FMT_C

#include "libmkb.h"
#include "fmt.h"

/* ASCII hex digit lookup table: 48='0' ..  57='9', 97='a' .. 102='f' */
static const u8 HEX_ASCII[16] = {
    48, 49, 50, 51,
    52, 53, 54, 55,
    56, 57, 97, 98,
    99, 100, 101, 102,
};

/* Append a newline to the end of string buffer str */
static void fmt_newline(mk_str_t * str) {
    if(str->len + 1 < MK_StrBufSize) {
        str->buf[str->len] = '\n';
        str->len += 1;
    }
}

/* Concatenate source string src onto the end of destination string dst. */
/* If dst lacks room to fit all of src, copy as many bytes as will fit.  */
static void fmt_concat(mk_str_t * dst, mk_str_t * src) {
    /* Calculate maximum number of bytes that can be copied */
    u32 len_both = src->len + dst->len;
    u8 last = len_both < MK_StrBufSize ? (u8) len_both : (u8) MK_StrBufSize;
    /* Copy that many bytes */
    u8 i, j;
    for(i = dst->len, j = 0; i <= last; i++, j++) {
        dst->buf[i] = src->buf[j];
    }
    dst->len = last;
}

/* Format n spaces into the end of the string buffer. */
static void fmt_spaces(mk_str_t * str, u8 n) {
    if(str->len + n < MK_StrBufSize) {
        int i;
        for(i = 0; i < n; i++) {
            str->buf[str->len + i] = (u8) ' ';
        }
        str->len += n;
    }
}

/* Format (8-bit clean copy) a byte into the end of the string buffer. */
static void fmt_raw_byte(mk_str_t * str, u8 data) {
    if(str->len + 1 < MK_StrBufSize) {
        str->buf[str->len] = data;
        str->len += 1;
    }
}

/* Hex-format a byte in '%02x' format into the end of the string buffer. */
static void fmt_hex_u8(mk_str_t * str, u8 data) {
    if(str->len + 2 < MK_StrBufSize) {
        str->buf[str->len    ] = HEX_ASCII[(data >> 4) & 0x0f];
        str->buf[str->len + 1] = HEX_ASCII[ data       & 0x0f];
        str->len += 2;
    }
}

/* Hex-format a halfword (u16) in '%04x' format into string buffer. */
static void fmt_hex_u16(mk_str_t * str, u16 data) {
    if(str->len + 4 < MK_StrBufSize) {
        str->buf[str->len    ] = HEX_ASCII[(data >> 12) & 0x0f];
        str->buf[str->len + 1] = HEX_ASCII[(data >>  8) & 0x0f];
        str->buf[str->len + 2] = HEX_ASCII[(data >>  4) & 0x0f];
        str->buf[str->len + 3] = HEX_ASCII[ data        & 0x0f];
        str->len += 4;
    }
}

#endif /* LIBMKB_FMT_C */
