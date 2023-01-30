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

#ifdef PLAN_9
#  include <u.h>
#else
#  include <stdint.h>
#endif

#include "libmkb.h"
#include "op.c"
#include "vm.c"
#include "autogen.c"

int mk_load_rom(const u8 * code, u32 code_len_bytes) {
    mk_context_t ctx = {
        0,       /* err */
        10,      /* base */
        0,       /* A */
        0,       /* B */
        0,       /* T */
        0,       /* S */
        0,       /* R */
        MK_Heap, /* PC */
        0,       /* DSDEEP */
        0,       /* RSDEEP */
        {0},     /* DSTACK[] */
        {0},     /* RSTACK[] */
        {0},     /* RAM */
        {0},     /* InBuf */
        {0},     /* OutBuf */
        0,       /* echo */
        0,       /* halted */
        0,       /* HoldStdout */
        0,       /* IOLOAD_depth */
        0,       /* IOLOAD_fail */
        0,       /* FOPEN_FILE */
        0,       /* DbgTraceEnable */
    };
    /* Copy code to RAM */
    int i;
    int n = code_len_bytes <= MK_HeapMax ? code_len_bytes : MK_HeapMax;
    for(i = 0; i <= n; i++) {
        ctx.RAM[i] = code[i];
    }
    /* Start clocking the VM from the boot vector */
    autogen_step(&ctx);
    return 0;
}

#endif /* LIBMKB_C */
