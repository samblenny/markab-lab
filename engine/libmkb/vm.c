/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_VM_C
#define LIBMKB_VM_C

#include "libmkb.h"
#include "autogen.h"
#include "vm.h"

static u8 vm_next_instruction(mk_context_t * ctx) {
    /* TODO: make this work properly */
    u8 instruction = ctx->RAM[ctx->PC];
    ctx->PC += 1;
    return instruction;
}

static void vm_irq_err(mk_context_t * ctx, u8 error_code) {
    /* TODO: implement this */
    mk_host_log_error(error_code);
}

#endif /* LIBMKB_VM_C */
