/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * THIS FILE IS AUTOMATICALLY GENERATED
 * DO NOT MAKE EDITS HERE
 * See codegen.py for details
 *
 * NOTE: This relies on typedefs from libmkb.h. This does not inlcude libmkb.h
 *       so you need to arrange for that on your own. Doing it this way makes
 *       it easier to build on POSIX systems and Plan 9 without a lot of chaos
 *       around different platform assumptions about header include paths.
 */
#ifndef LIBMKB_AUTOGEN_H
#define LIBMKB_AUTOGEN_H

/* Markab VM opcode constants */
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
#define MK_DUMP   (0x0d  /* 13 */)
#define MK_KEY    (0x0e  /* 14 */)
#define MK_IORH   (0x0f  /* 15 */)
#define MK_MTR    (0x10  /* 16 */)
#define MK_R      (0x11  /* 17 */)
#define MK_CALL   (0x12  /* 18 */)
#define MK_PC     (0x13  /* 19 */)
#define MK_MTE    (0x14  /* 20 */)
#define MK_LB     (0x15  /* 21 */)
#define MK_SB     (0x16  /* 22 */)
#define MK_LH     (0x17  /* 23 */)
#define MK_SH     (0x18  /* 24 */)
#define MK_LW     (0x19  /* 25 */)
#define MK_SW     (0x1a  /* 26 */)
#define MK_ADD    (0x1b  /* 27 */)
#define MK_SUB    (0x1c  /* 28 */)
#define MK_MUL    (0x1d  /* 29 */)
#define MK_DIV    (0x1e  /* 30 */)
#define MK_MOD    (0x1f  /* 31 */)
#define MK_SLL    (0x20  /* 32 */)
#define MK_SRL    (0x21  /* 33 */)
#define MK_SRA    (0x22  /* 34 */)
#define MK_INV    (0x23  /* 35 */)
#define MK_XOR    (0x24  /* 36 */)
#define MK_OR     (0x25  /* 37 */)
#define MK_AND    (0x26  /* 38 */)
#define MK_GT     (0x27  /* 39 */)
#define MK_LT     (0x28  /* 40 */)
#define MK_EQ     (0x29  /* 41 */)
#define MK_NE     (0x2a  /* 42 */)
#define MK_ZE     (0x2b  /* 43 */)
#define MK_INC    (0x2c  /* 44 */)
#define MK_DEC    (0x2d  /* 45 */)
#define MK_EMIT   (0x2e  /* 46 */)
#define MK_DOT    (0x2f  /* 47 */)
#define MK_IODH   (0x30  /* 48 */)
#define MK_IOD    (0x31  /* 49 */)
#define MK_RDROP  (0x32  /* 50 */)
#define MK_DROP   (0x33  /* 51 */)
#define MK_DUP    (0x34  /* 52 */)
#define MK_OVER   (0x35  /* 53 */)
#define MK_SWAP   (0x36  /* 54 */)
#define MK_MTA    (0x37  /* 55 */)
#define MK_LBA    (0x38  /* 56 */)
#define MK_LBAI   (0x39  /* 57 */)
#define MK_AINC   (0x3a  /* 58 */)
#define MK_ADEC   (0x3b  /* 59 */)
#define MK_A      (0x3c  /* 60 */)
#define MK_MTB    (0x3d  /* 61 */)
#define MK_LBB    (0x3e  /* 62 */)
#define MK_LBBI   (0x3f  /* 63 */)
#define MK_SBBI   (0x40  /* 64 */)
#define MK_BINC   (0x41  /* 65 */)
#define MK_BDEC   (0x42  /* 66 */)
#define MK_B      (0x43  /* 67 */)
#define MK_TRUE   (0x44  /* 68 */)
#define MK_FALSE  (0x45  /* 69 */)

/* Markab VM opcode dictionary */
#define MK_OPCODES_LEN (70)
static const char * const opcodes[MK_OPCODES_LEN];

/* Markab VM memory map */
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
#define MK_IRQERR    (0xE134)
#define MK_IB        (0xE200)
#define MK_Pad       (0xE300)
#define MK_Scratch   (0xE400)
#define MK_MemMax    (0xFFFF)

/* Markab language enum codes */
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

/* Markab language core vocabulary */
#define MK_CORE_VOC_LEN (105)
#define MK_VOC_ITEM_NAME_LEN (16)
typedef struct mk_voc_item {
    const char * const name[MK_VOC_ITEM_NAME_LEN];
    const u8 type_code;
    const u32 value;
} mk_voc_item_t;
static const mk_voc_item_t mk_core_voc[MK_CORE_VOC_LEN];

static void autogen_step(mk_context_t * ctx);

#endif /* LIBMKB_AUTOGEN_H */
