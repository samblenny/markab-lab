/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_VM_H
#define LIBMKB_VM_H

#define MK_ERR_D_OVER          (1  /* Data stack overflow */)
#define MK_ERR_D_UNDER         (2  /* Data stack underflow */)
#define MK_ERR_BAD_ADDRESS     (3  /* Expected vaild address */)
#define MK_ERR_BOOT_OVERFLOW   (4  /* ROM image is too big */)
#define MK_ERR_BAD_INSTRUCTION (5  /* Expected a valid opcode */)
#define MK_ERR_R_OVER          (6  /* Return stack overflow */)
#define MK_ERR_R_UNDER         (7  /* Return stack underflow */)
#define MK_ERR_MAX_CYCLES      (8  /* Code ran for too many clock cycles */)
#define MK_ERR_FILEPATH        (9  /* Filepath failed sandbox checks */)
#define MK_ERR_FILE_NOT_FOUND  (10  /* Unable to open specified filepath */)
#define MK_ERR_UNKNOWN         (11  /* Outer interpreter found unknown word */)
#define MK_ERR_NEST            (12  /* Unbalanced }if or }for */)
#define MK_ERR_IOLOAD_DEPTH    (13  /* Too many levels of nested `load ...` */)
#define MK_ERR_BAD_PC_ADDR     (14  /* Program counter went out of range */)
#define MK_ERR_IOLOAD_FAIL     (15  /* Error while loading a file */)
#define MK_ERR_NO_OPEN_FILE    (16  /* Requested operation depends on FOPEN */)
#define MK_ERR_OPEN_FILE       (17  /* Attempt to FOPEN when file is open */)
#define MK_ERR_FILE_IO_FAIL    (18  /* Misc errors from host OS file IO API */)
#define MK_ERR_UTF8            (19  /* Error decoding UTF-8 string */)

static u8 vm_next_instruction(mk_context_t * ctx);

/* Log an error code to whatever device serves as the VM's stderr */
static void vm_irq_err(u8 error_code);

/* Write a buffer of bytes to whatever device serves as the VM's stdout */
static void vm_stdout_write(const mk_str_t * str);

#endif /* LIBMKB_VM_H */
