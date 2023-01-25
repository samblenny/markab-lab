// Copyright (c) 2023 Sam Blenny
// SPDX-License-Identifier: MIT
//
// THIS FILE IS AUTOMATICALLY GENERATED
// DO NOT MAKE EDITS HERE
// See codegen.py for details
//
#ifndef LIBMKB_AUTOGEN_H
#define LIBMKB_AUTOGEN_H

#include <stdint.h>

// Shorthand integer typedefs to save on typing
typedef  uint8_t  u8;
typedef   int8_t  i8;
typedef uint16_t u16;
typedef  int16_t i16;
typedef  int32_t i32;
typedef uint32_t u32;

// Markab VM opcode constants
#define MK_NOP    (0x00  /*  0 */)
#define MK_RESET  (0x01  /*  1 */)
#define MK_JMP    (0x02  /*  2 */)
#define MK_JAL    (0x03  /*  3 */)
#define MK_RET    (0x04  /*  4 */)
#define MK_BZ     (0x05  /*  5 */)
#define MK_BFOR   (0x06  /*  6 */)
#define MK_U8     (0x07  /*  7 */)
#define MK_U16    (0x08  /*  8 */)
#define MK_I32    (0x09  /*  9 */)
#define MK_HALT   (0x0a  /* 10 */)
#define MK_TRON   (0x0b  /* 11 */)
#define MK_TROFF  (0x0c  /* 12 */)
#define MK_IODUMP (0x0d  /* 13 */)
#define MK_IOKEY  (0x0e  /* 14 */)
#define MK_IORH   (0x0f  /* 15 */)
#define MK_IOLOAD (0x10  /* 16 */)
#define MK_FOPEN  (0x11  /* 17 */)
#define MK_FREAD  (0x12  /* 18 */)
#define MK_FWRITE (0x13  /* 19 */)
#define MK_FSEEK  (0x14  /* 20 */)
#define MK_FTELL  (0x15  /* 21 */)
#define MK_FTRUNC (0x16  /* 22 */)
#define MK_FCLOSE (0x17  /* 23 */)
#define MK_MTR    (0x18  /* 24 */)
#define MK_R      (0x19  /* 25 */)
#define MK_CALL   (0x1a  /* 26 */)
#define MK_PC     (0x1b  /* 27 */)
#define MK_MTE    (0x1c  /* 28 */)
#define MK_LB     (0x1d  /* 29 */)
#define MK_SB     (0x1e  /* 30 */)
#define MK_LH     (0x1f  /* 31 */)
#define MK_SH     (0x20  /* 32 */)
#define MK_LW     (0x21  /* 33 */)
#define MK_SW     (0x22  /* 34 */)
#define MK_ADD    (0x23  /* 35 */)
#define MK_SUB    (0x24  /* 36 */)
#define MK_MUL    (0x25  /* 37 */)
#define MK_DIV    (0x26  /* 38 */)
#define MK_MOD    (0x27  /* 39 */)
#define MK_SLL    (0x28  /* 40 */)
#define MK_SRL    (0x29  /* 41 */)
#define MK_SRA    (0x2a  /* 42 */)
#define MK_INV    (0x2b  /* 43 */)
#define MK_XOR    (0x2c  /* 44 */)
#define MK_OR     (0x2d  /* 45 */)
#define MK_AND    (0x2e  /* 46 */)
#define MK_GT     (0x2f  /* 47 */)
#define MK_LT     (0x30  /* 48 */)
#define MK_EQ     (0x31  /* 49 */)
#define MK_NE     (0x32  /* 50 */)
#define MK_ZE     (0x33  /* 51 */)
#define MK_INC    (0x34  /* 52 */)
#define MK_DEC    (0x35  /* 53 */)
#define MK_IOEMIT (0x36  /* 54 */)
#define MK_IODOT  (0x37  /* 55 */)
#define MK_IODH   (0x38  /* 56 */)
#define MK_IOD    (0x39  /* 57 */)
#define MK_RDROP  (0x3a  /* 58 */)
#define MK_DROP   (0x3b  /* 59 */)
#define MK_DUP    (0x3c  /* 60 */)
#define MK_OVER   (0x3d  /* 61 */)
#define MK_SWAP   (0x3e  /* 62 */)
#define MK_MTA    (0x3f  /* 63 */)
#define MK_LBA    (0x40  /* 64 */)
#define MK_LBAI   (0x41  /* 65 */)
#define MK_AINC   (0x42  /* 66 */)
#define MK_ADEC   (0x43  /* 67 */)
#define MK_A      (0x44  /* 68 */)
#define MK_MTB    (0x45  /* 69 */)
#define MK_LBB    (0x46  /* 70 */)
#define MK_LBBI   (0x47  /* 71 */)
#define MK_SBBI   (0x48  /* 72 */)
#define MK_BINC   (0x49  /* 73 */)
#define MK_BDEC   (0x4a  /* 74 */)
#define MK_B      (0x4b  /* 75 */)
#define MK_TRUE   (0x4c  /* 76 */)
#define MK_FALSE  (0x4d  /* 77 */)

// Markab VM opcode dictionary
#define MK_OPCODES_LEN (78)
static const char * const opcodes[MK_OPCODES_LEN];

// Markab VM memory map
#define MK_Heap      (0x0000)
#define MK_HeapRes   (0xE000)
#define MK_HeapMax   (0xE0FF)
#define MK_DP        (0xE100)
#define MK_IN        (0xE104)
#define MK_CORE_V    (0xE108)
#define MK_EXT_V     (0xE10C)
#define MK_MODE      (0xE110)
#define MK_LASTCALL  (0xE118)
#define MK_NEST      (0xE11C)
#define MK_BASE      (0xE120)
#define MK_EOF       (0xE124)
#define MK_LASTWORD  (0xE128)
#define MK_IRQRX     (0xE12C)
#define MK_OK_EN     (0xE130)
#define MK_LOADNEST  (0xE134)
#define MK_IRQERR    (0xE138)
#define MK_IB        (0xE200)
#define MK_Pad       (0xE300)
#define MK_Scratch   (0xE400)
#define MK_MemMax    (0xFFFF)

// Markab language enum codes
#define MK_T_VAR       (0)
#define MK_T_CONST     (1)
#define MK_T_OP        (2)
#define MK_T_OBJ       (3)
#define MK_T_IMM       (4)
#define MK_MODE_INT    (0)
#define MK_MODE_COM    (1)
#define MK_ErrUnknown  (11)
#define MK_ErrNest     (12)
#define MK_ErrFilepath (9)
#define MK_HashA       (7)
#define MK_HashB       (8)
#define MK_HashC       (38335)
#define MK_HashBins    (64)
#define MK_HashMask    (63)

// Markab language core vocabulary
#define MK_CORE_VOC_LEN (114)
#define MK_VOC_ITEM_NAME_LEN (16)
typedef struct mk_voc_item {
    const char * const name[MK_VOC_ITEM_NAME_LEN];
    const uint8_t type_code;
    const u32 value;
} mk_voc_item_t;
static const mk_voc_item_t mk_core_voc[MK_CORE_VOC_LEN];

// VM context struct for holding state of registers and RAM
#define MK_BufMax (256)
typedef struct mk_context {
    u8  err;               // Error register (don't confuse with ERR opcode!)
    u8  base;              // number Base for debug printing
    i32 A;                 // register for source address or scratch
    i32 B;                 // register for destination addr or scratch
    i32 T;                 // Top of data stack
    i32 S;                 // Second on data stack
    i32 R;                 // top of Return stack
    u32 PC;                // Program Counter
    u8  DSDeep;            // Data Stack Depth (count include T and S)
    u8  RSDeep;            // Return Stack Depth (count inlcudes R)
    i32 DStack[16];        // Data Stack
    i32 RStack[16];        // Return Stack
    u8  RAM[MK_MemMax+1];  // Random Access Memory
    u8  InBuf[MK_BufMax];  // Input buffer
    u8  OutBuf[MK_BufMax]; // Output buffer
    u8  echo;              // Echo depends on tty vs pip, etc.
    u8  halted;            // Flag to track halt (used for `bye`)
    u8  HoldStdout;        // Flag to use holding buffer for stdout
    u8  IOLOAD_depth;      // Nesting level for io_load_file()
    u8  IOLOAD_fail;       // Flag indicating an error during io_load_file()
    u8  FOPEN_file;        // File (if any) that was opened by FOPEN 
    u8  DbgTraceEnable;    // Debug trace on/off
} mk_context_t;

// Maximum number of cycles allowed before infinite loop error triggers
#define MK_MAX_CYCLES (65535)

static void autogen_step(mk_context_t * ctx);

#endif /* LIBMKB_AUTOGEN_H */
