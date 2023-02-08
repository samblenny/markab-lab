/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Compile source code to bytecode for the Markab bytecode interpreter.
 */
#ifndef LIBMKB_COMP_C
#define LIBMKB_COMP_C

#include "libmkb.h"
#include "autogen.h"
#include "comp.h"

/*
 * TODO: Write a compiler
 */

/* Compile Markab Script source from text into bytecode in ctx.RAM.       */
/* Compile error details get logged using mk_host_*() Host API functions. */
/* Returns: 1 for success, 0 for failure                                  */
int comp_compile_src(mk_context_t *ctx, const u8 * text, u32 text_len) {
    u32 i;
    for(i = 0; i < text_len; i++) {
        if(text[i] == 0) {
            ctx->RAM[0] = 0;
        }
    }
    return 0;
}


#endif /* LIBMKB_COMP_C */
