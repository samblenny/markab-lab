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
    u8  err;               /* Error register (don't confuse with ERR opcode!) */
    u8  base;              /* number Base for debug printing */
    i32 A;                 /* register for source address or scratch */
    i32 B;                 /* register for destination addr or scratch */
    i32 T;                 /* Top of data stack */
    i32 S;                 /* Second on data stack */
    i32 R;                 /* top of Return stack */
    u32 PC;                /* Program Counter */
    u8  DSDeep;            /* Data Stack Depth (count include T and S) */
    u8  RSDeep;            /* Return Stack Depth (count inlcudes R) */
    i32 DStack[16];        /* Data Stack */
    i32 RStack[16];        /* Return Stack */
    u8  RAM[MK_RamMax+1];  /* Random Access Memory */
    u8  InBuf[MK_BufMax];  /* Input buffer */
    u8  OutBuf[MK_BufMax]; /* Output buffer */
    u8  echo;              /* Echo depends on tty vs pip, etc. */
    u8  halted;            /* Flag to track halt (used for `bye`) */
    u8  HoldStdout;        /* Flag to use holding buffer for stdout */
    u8  IOLOAD_depth;      /* Nesting level for io_load_file() */
    u8  IOLOAD_fail;       /* Flag indicating an error during io_load_file() */
    u8  FOPEN_file;        /* File (if any) that was opened by FOPEN  */
    u8  DbgTraceEnable;    /* Debug trace on/off */
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
 * Public Interface: Functions provided by libmkb
 */

int mk_load_rom(const u8 * code, u32 code_len_bytes);


/*
 * Public Interface: Functions libmkb expects its front end to export
 */

/* Write an error code to stderr */
extern void mk_host_log_error(u8 error_code);

/* Write length bytes from byte buffer buf to stdout */
extern void mk_host_stdout_write(const void * buf, int length);

/* Read byte from stdin to *data, returning 0 for success or 1 for EOF */
extern u8 mk_host_getchar(u8 * data);

/* Write byte to stdout */
extern void mk_host_putchar(u8 data);


#endif /* LIBMKB_H */
