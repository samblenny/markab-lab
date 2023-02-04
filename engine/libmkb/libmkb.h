/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBMKB_H
#define LIBMKB_H

/*
 * Integer Types (CAUTION: platform-specific quirkiness)
 */

#ifdef PLAN_9
  /* This is for Plan 9.
   * You need to do a `#include <u.h>` somewhere before this.
   */
  typedef char i8;
  typedef short i16;
  typedef int i32;
  typedef u8int u8;
  typedef u16int u16;
  typedef u32int u32;
#else
  /* This is for POSIX systems.
   * You need to do a `#include <stdint.h>` somewhere before this.
   */
  typedef  uint8_t  u8;
  typedef   int8_t  i8;
  typedef uint16_t u16;
  typedef  int16_t i16;
  typedef  int32_t i32;
  typedef uint32_t u32;
#endif


/*
 * VM Internal Types and Constants
 */

/* VM context struct for holding state of registers and RAM */
#define MK_BufMax (256)
#define MK_RamMax (65535)
typedef struct mk_context {
    u32 DSDeep;            /* Data Stack Depth (count includes T and S) */
    i32 T;                 /* Top of data stack */
    i32 S;                 /* Second on data stack */
    i32 DStack[16];        /* Data Stack */
    u32 RSDeep;            /* Return Stack Depth (count inlcudes R) */
    i32 R;                 /* Top of Return stack */
    i32 RStack[16];        /* Return Stack */
    u32 PC;                /* Program Counter */
    u8  halted;            /* Flag to track halt (used for `bye`) */
    u8  base;              /* Number base for parsing and print formatting */
    u8  RAM[MK_RamMax+1];  /* Random Access Memory */
    u8  err;               /* Error code register */
    u8  DbgTraceEnable;    /* Debug trace on/off status */
} mk_context_t;

/* Counted string buffer typedef */
#define MK_StrBufSize (255)
typedef struct mk_str {
    u8 len;
    u8 buf[MK_StrBufSize];
} mk_str_t;

/* Maximum number of cycles allowed before infinite loop error triggers */
#define MK_MAX_CYCLES (65535)


/*
 * VM Error status codes
 */

#define MK_ERR_OK              (0  /* OK: No errors */)
#define MK_ERR_D_OVER          (1  /* Data stack overflow */)
#define MK_ERR_D_UNDER         (2  /* Data stack underflow */)
#define MK_ERR_R_OVER          (3  /* Return stack overflow */)
#define MK_ERR_R_UNDER         (4  /* Return stack underflow */)
#define MK_ERR_BAD_ADDRESS     (5  /* Expected vaild address */)
#define MK_ERR_BAD_INSTRUCTION (6  /* Expected a valid opcode */)
#define MK_ERR_MAX_CYCLES      (7  /* Code ran for too many clock cycles */)
#define MK_ERR_UNKNOWN         (8  /* Outer interpreter found unknown word */)
#define MK_ERR_NEST            (9  /* Unbalanced }if or }for */)


/*
 * Public Interface: Functions provided by libmkb
 */

/* Load code (a rom image) into RAM, run it, and return VM's error code.
 * Error code MK_ERR_OK means there were no errrors.
 */
int mk_load_rom(const u8 * code, u32 code_len_bytes);


/*
 * Public Interface: Functions libmkb expects its front end to export
 */

/* Write an error code to stderr */
extern void mk_host_log_error(u8 error_code);

/* Write length bytes from byte buffer buf to stdout */
extern void mk_host_stdout_write(const void * buf, int length);

/* Write byte to stdout */
extern void mk_host_putchar(u8 data);


#endif /* LIBMKB_H */
