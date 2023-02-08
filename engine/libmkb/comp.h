/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Compile source code to bytecode for the Markab bytecode interpreter.
 */
#ifndef LIBMKB_COMP_H
#define LIBMKB_COMP_H

/* Parameters for multiply-with-carry (mwc) string hashing function */
#define comp_HashA 7
#define comp_HashB 8
#define comp_HashC 38335
#define comp_HashBins 64
#define comp_HashMask 63

/* Compile Markab Script source from text into bytecode in ctx.RAM.       */
/* Compile error details get logged using mk_host_*() Host API functions. */
/* Returns: 1 for success, 0 for failure                                  */
int comp_compile_src(mk_context_t *ctx, const u8 * text, u32 text_len);


#endif /* LIBMKB_COMP_H */
