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
#define MK_HALT   (0x02  /*  2 */)
#define MK_TRON   (0x03  /*  3 */)
#define MK_TROFF  (0x04  /*  4 */)
#define MK_MTE    (0x05  /*  5 */)
#define MK_U8     (0x06  /*  6 */)
#define MK_U16    (0x07  /*  7 */)
#define MK_I32    (0x08  /*  8 */)
#define MK_STR    (0x09  /*  9 */)
#define MK_BZ     (0x0a  /* 10 */)
#define MK_JMP    (0x0b  /* 11 */)
#define MK_JAL    (0x0c  /* 12 */)
#define MK_RET    (0x0d  /* 13 */)
#define MK_CALL   (0x0e  /* 14 */)
#define MK_LB     (0x0f  /* 15 */)
#define MK_SB     (0x10  /* 16 */)
#define MK_LH     (0x11  /* 17 */)
#define MK_SH     (0x12  /* 18 */)
#define MK_LW     (0x13  /* 19 */)
#define MK_SW     (0x14  /* 20 */)
#define MK_INC    (0x15  /* 21 */)
#define MK_DEC    (0x16  /* 22 */)
#define MK_ADD    (0x17  /* 23 */)
#define MK_SUB    (0x18  /* 24 */)
#define MK_MUL    (0x19  /* 25 */)
#define MK_DIV    (0x1a  /* 26 */)
#define MK_MOD    (0x1b  /* 27 */)
#define MK_SLL    (0x1c  /* 28 */)
#define MK_SRL    (0x1d  /* 29 */)
#define MK_SRA    (0x1e  /* 30 */)
#define MK_INV    (0x1f  /* 31 */)
#define MK_XOR    (0x20  /* 32 */)
#define MK_OR     (0x21  /* 33 */)
#define MK_AND    (0x22  /* 34 */)
#define MK_GT     (0x23  /* 35 */)
#define MK_LT     (0x24  /* 36 */)
#define MK_EQ     (0x25  /* 37 */)
#define MK_NE     (0x26  /* 38 */)
#define MK_ZE     (0x27  /* 39 */)
#define MK_TRUE   (0x28  /* 40 */)
#define MK_FALSE  (0x29  /* 41 */)
#define MK_DROP   (0x2a  /* 42 */)
#define MK_DUP    (0x2b  /* 43 */)
#define MK_OVER   (0x2c  /* 44 */)
#define MK_SWAP   (0x2d  /* 45 */)
#define MK_PC     (0x2e  /* 46 */)
#define MK_R      (0x2f  /* 47 */)
#define MK_MTR    (0x30  /* 48 */)
#define MK_RDROP  (0x31  /* 49 */)
#define MK_EMIT   (0x32  /* 50 */)
#define MK_HEX    (0x33  /* 51 */)
#define MK_DECIMAL (0x34  /* 52 */)
#define MK_BASE   (0x35  /* 53 */)
#define MK_PRINT  (0x36  /* 54 */)
#define MK_CR     (0x37  /* 55 */)
#define MK_DOT    (0x38  /* 56 */)
#define MK_DOTS   (0x39  /* 57 */)
#define MK_DOTSH  (0x3a  /* 58 */)
#define MK_DOTRH  (0x3b  /* 59 */)
#define MK_DUMP   (0x3c  /* 60 */)

/* Markab VM opcode dictionary */
#define MK_OPCODES_LEN (61)
static const char * const opcodes[MK_OPCODES_LEN];

/* Markab VM memory map */
#define MK_Heap      (0x0000)
#define MK_HeapRes   (0xE000)
#define MK_HeapMax   (0xE0FF)
#define MK_DP        (0xE100)
#define MK_MemMax    (0xFFFF)

/* Markab language enum codes */
#define MK_T_VAR       (0)
#define MK_T_CONST     (1)
#define MK_T_OP        (2)
#define MK_T_OBJ       (3)
#define MK_T_IMM       (4)

/* Markab language core vocabulary */
#define MK_CORE_VOC_LEN (71)
#define MK_VOC_ITEM_NAME_LEN (16)
typedef struct mk_voc_item {
    const char * const name[MK_VOC_ITEM_NAME_LEN];
    const u8 type_code;
    const u32 value;
} mk_voc_item_t;
static const mk_voc_item_t mk_core_voc[MK_CORE_VOC_LEN];

#endif /* LIBMKB_AUTOGEN_H */
