/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_VM_C
#define LIBMKB_VM_C

#include "autogen.h"

static u8 vm_next_instruction(mk_context_t * ctx) {
    /* TODO: make this work properly */
    return MK_HALT;
}

static void vm_irq_err(mk_context_t * ctx, u8 error_code) {
    /* TODO: implement this */
}

#endif /* LIBMKB_VM_C */
