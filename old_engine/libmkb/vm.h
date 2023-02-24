/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_VM_H
#define LIBMKB_VM_H

static u8 vm_next_instruction(mk_context_t * ctx);

/* Log an error code to whatever device serves as the VM's stderr */
static void vm_irq_err(mk_context_t * ctx, u8 error_code);

/* Write a buffer of bytes to whatever device serves as the VM's stdout */
static void vm_stdout_write(const mk_str_t * str);

#endif /* LIBMKB_VM_H */
