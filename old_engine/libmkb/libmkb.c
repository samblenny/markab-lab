/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * libmkb: Library providing a Markab VM with no external dependencies. This
 * is meant to be called from front-end code which uses some suitable API on
 * the host OS to provide a means of input and output.
 *
 * There is some unusual stuff here, but not without compelling reasons:
 * 1. Yes, I know this file includes .c files. There's a good reason for it.
 * 2. Yes, I know about link time optimization. But, no, that would not be a
 *    sufficient solution here. For this situation, I need inlining.
 *
 * This arrangement of including .c files allows for separating code into files
 * for easier navigation and editing while still letting the compiler generate
 * efficient inlined code. This is particularly important for the big switch
 * statement in the bytecode interpreter loop (see autogen.c).
 *
 * Normally, to get inlining, the opcode implementations and the switch
 * statement would need to be in the same file. But, in this case, the switch
 * is autogenerated while the opcode implementations are hand-edited. So, they
 * need to be in separate files. Compiling the switch and opcode implementation
 * functions into separate object files, then linking with LTO, would generate
 * less efficient code compared to inlining. The .c #includes allow inlining.
 *
 * Another advantage of doing it this way, and declaring most stuff static, is
 * this allows using non-prefixed names internally without polluting the
 * namespace of exported symbols. Using shorter names helps a lot to keep the
 * code readable.
 */


#ifndef LIBMKB_C
#define LIBMKB_C

#include <stdint.h>
#ifndef WASM_MEMCPY
#   include <string.h>  /* memcpy(), memset() */
#else
/*****************************************************************************/
/* DIY stdlib replacement: this works around lack of wasm32 standard library */
/*****************************************************************************/
    void *memcpy(void *dest, const void *src, unsigned long n) {
        u32 i;
        for(i = 0; i < n; i++) {
            ((u8 *)dest)[i] = ((u8 *)src)[i];
        }
        return dest;
    }
    void *memset(void *s, int c, unsigned long n) {
        u32 i;
        for(i = 0; i < n; i++) {
            ((u8 *)s)[i] = c;
        }
        return s;
    }
/*****************************************************************************/
#endif
#include "libmkb.h"
#include "fmt.c"
#include "op.c"
#include "vm.c"
#include "autogen.c"
#include "comp.c"


/* Load and run a markab VM ROM image.
 * Returns: value of VM err register (0 means OK, see vm.h for other codes)
 */
int mk_load_rom(const u8 * code, u32 code_len_bytes) {
    mk_context_t ctx = {
        0,       /* DSDEEP */
        0,       /* T */
        0,       /* S */
        {0},     /* DSTACK[] */
        0,       /* RSDEEP */
        0,       /* R */
        {0},     /* RSTACK[] */
        0,       /* PC */
        0,       /* DP */
        {0},     /* RAM */
        0,       /* halted */
        0,       /* err */
    };
    /* Copy code from ROM to RAM, truncating whatever doesn't fit. This is
     * meant to allow for the possibility of a ROM file containing code
     * followed by images, fonts, audio samples, etc which can be paged into
     * ROM.
     *
     * TODO: Implement paging opcodes to access the high-area of large ROMs.
     */
    u32 n = code_len_bytes <= MK_MEM_MAX ? code_len_bytes : MK_MEM_MAX;
    memcpy((void *)ctx.RAM, (void *)code, n);
    /* For small ROMs, fill rest of RAM with NOP instructions */
    if(n < sizeof(ctx.RAM)) {
        memset((void *)(&ctx.RAM[n]), MK_NOP, sizeof(ctx.RAM) - n);
    }
    /* Start clocking the VM from the boot vector */
    autogen_step(&ctx);
    /* Return value of the VM's error register */
    return ctx.err;
}

/* Compile Markab Script source code, run it, and return VM's error code. */
/* Error code MK_ERR_OK means there were no errrors.                      */
int mk_compile_and_run(const u8 * text, u32 text_len_bytes) {
    mk_context_t ctx = {
        0,       /* DSDEEP */
        0,       /* T */
        0,       /* S */
        {0},     /* DSTACK[] */
        0,       /* RSDEEP */
        0,       /* R */
        {0},     /* RSTACK[] */
        0,       /* PC */
        0,       /* DP */
        {0},     /* RAM */
        0,       /* halted */
        0,       /* err */
    };
    /* Zero VM RAM */
    memset((void *)ctx.RAM, MK_NOP, sizeof(ctx.RAM));
    /* Compile the Markab Script source */
    if(comp_compile_src(&ctx, text, text_len_bytes)) {
        /* Start clocking the VM from the boot vector */
        autogen_step(&ctx);
    } else {
        ctx.err = MK_ERR_COMPILE;
    }
    /* Return value of the VM's error register */
    return ctx.err;
}

#endif /* LIBMKB_C */