/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_VM_C
#define LIBMKB_VM_C

#include "libmkb.h"
#include "autogen.h"
#include "vm.h"

/* Fetch the next instruction for the bytecode interpreter */
static u8 vm_next_instruction(mk_context_t * ctx) {
    u8 instruction = ctx->RAM[ctx->PC];
    /* CAUTION! This relies on PC being of type u16 with a range that exactly
     *          matches the RAM array size of 65536. It's designed to let PC
     *          overflow and wrap around from 65535 back down to 0 in case
     *          code flow goes haywire. Doing it this way might be a bad idea.
     *          I made this choice for now because I want the inner loop of
     *          the bytecode interpreter to be fast. Maybe this is too fast?
     *
     * TODO: Should I add an error check for the PC going out of range?
     */
    ctx->PC += 1;
    return instruction;
}

/* Log an error code to whatever device serves as the VM's stderr */
static void vm_irq_err(mk_context_t * ctx, u8 error_code) {
    mk_host_log_error(error_code);
    /* TODO: Do I need to halt the VM here? */
}

/* Write a buffer of bytes to whatever device serves as the VM's stdout */
static void vm_stdout_write(const mk_str_t * str) {
    mk_host_stdout_write((const char *)&str->buf, str->len);
    /* TODO: Should I verify the expected number of bytes were written? */
}

#endif /* LIBMKB_VM_C */
