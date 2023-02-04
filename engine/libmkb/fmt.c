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

/* Hex-format i32 word in '%x' variable width format into string buffer.
 * Example for R = 0x00001f0f: 1f0f
 * Example for R = 0x00000001: 1
 */
static void fmt_hex(mk_str_t * str, u32 data) {
    /* If the data byte is 0, skip all the fancy stuff */
    if(data == 0) {
        fmt_raw_byte(str, '0');
        return;
    }
    /* Otherwise, compute the full 8-digit hex string */
    u8 nibbles[8] = {
        (data >> 28) & 0x0f,
        (data >> 24) & 0x0f,
        (data >> 20) & 0x0f,
        (data >> 16) & 0x0f,
        (data >> 12) & 0x0f,
        (data >>  8) & 0x0f,
        (data >>  4) & 0x0f,
        (data      ) & 0x0f,
    };
    /* Then filter out the leading zeros and append what's left to str */
    u8 skip_leading_zeros = 1;
    u8 i;
    for(i = 0; i < 8; i++) {
        if(skip_leading_zeros && nibbles[i] == 0) {
            continue;
        }
        skip_leading_zeros = 0;
        fmt_raw_byte(str, HEX_ASCII[nibbles[i]]);
    }
}

/* Format i32 into string buffer in variable width signed-decimal format. */
static void fmt_decimal(mk_str_t * str, i32 n) {
    /* If n is 0, skip all the fancy stuff */
    if(n == 0) {
        fmt_raw_byte(str, '0');
        return;
    }
    /* If n's is negative (sign bit set), append a '-' to str then negate n
     * CAUTION: Initially, I thought it might be fine to negate n as i32 with
     *          (-n). That worked okay on macOS at first glance. But, the
     *          conversion for 0x80000000 went haywire on Plan 9. Taking a
     *          closer look at edge-case behavior for 0x80000000 = -2147483648,
     *          it seems better to do `u32 x=n;` and negate with `(~x) + 1`.
     */
    u32 x = (u32) n;
    if(x >> 31) {
        fmt_raw_byte(str, '-');
        x = (~x) + 1;
    }
    /* Convert x to a list of base-10 digits. Range of n is -2,147,483,648 to
     * 2,147,483,647. Range of x = abs(n) is 0 to 2,147,483,648. So, allow for
     * up to 10 digits.
     */
    u8 digits[10];
    u8 i;
    for(i = 0; i < 10; i++) {
        digits[9-i] = (u8) (x % 10);
        x = (i32) (x / 10);
    }
    /* Then filter out the leading zeros and append what's left to str */
    u8 skip_leading_zeros = 1;
    for(i = 0; i < 10; i++) {
        if(skip_leading_zeros && digits[i] == 0) {
            continue;
        }
        skip_leading_zeros = 0;
        fmt_raw_byte(str, '0' + digits[i]);
    }
}

/* Append a copy of null-terminated cstring into string buffer str. */
static void fmt_cstring(mk_str_t * str, const char * cstring) {
    /* Calculate the margin of bytes available to receive copied characters */
    i32 margin = MK_StrBufSize - str->len;
    /* Copy bytes from ctring; stop for null or when margin is full */
    int i;
    for(i = 0; i < margin; i++) {
        char c = cstring[i];
        if(c == 0) {
            break;
        }
        str->buf[str->len] = c;
        str->len += 1;
    }
}

#endif /* LIBMKB_FMT_C */
