/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Minimalist DIY string formatting functions.
 *
 * NOTE: This relies on typedefs from libmkb.h. This does not inlcude libmkb.h
 *       so you need to arrange for that on your own. Doing it this way makes
 *       it easier to build on POSIX systems and Plan 9 without a lot of chaos
 *       around different platform assumptions about header include paths.
 */
#ifndef LIBMKB_FMT_H
#define LIBMKB_FMT_H

/* Append a newline to the end of string buffer str */
static void fmt_newline(mk_str_t * str);

/* Concatenate source string src onto the end of destination string dst. */
/* If dst lacks room to fit all of src, copy as many bytes as will fit.  */
static void fmt_concat(mk_str_t * dst, mk_str_t * src);

/* Format n spaces into the end of the string buffer. */
static void fmt_spaces(mk_str_t * str, u8 n);

/* Format (8-bit clean copy) a byte into the end of the string buffer. */
static void fmt_raw_byte(mk_str_t * str, u8 data);

/* Hex-format a byte in '%02x' format into string buffer. */
static void fmt_hex_u8(mk_str_t * str, u8 data);

/* Hex-format a halfword (u16) in '%04x' format into string buffer. */
static void fmt_hex_u16(mk_str_t * str, u16 data);


#endif /* LIBMKB_FMT_H */
