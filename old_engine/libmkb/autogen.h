/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * THIS FILE IS AUTOMATICALLY GENERATED
 * DO NOT MAKE EDITS HERE
 * See codegen.py for details
 */
#ifndef LIBMKB_AUTOGEN_H
#define LIBMKB_AUTOGEN_H

/* Markab VM opcode constants */
#define MK_NOP    (0x00  /*  0 */)
#define MK_HALT   (0x01  /*  1 */)
#define MK_U8     (0x02  /*  2 */)
#define MK_U16    (0x03  /*  3 */)
#define MK_I32    (0x04  /*  4 */)
#define MK_STR    (0x05  /*  5 */)
#define MK_BZ     (0x06  /*  6 */)
#define MK_BNZ    (0x07  /*  7 */)
#define MK_JMP    (0x08  /*  8 */)
#define MK_JAL    (0x09  /*  9 */)
#define MK_RET    (0x0a  /* 10 */)
#define MK_CALL   (0x0b  /* 11 */)
#define MK_LB     (0x0c  /* 12 */)
#define MK_SB     (0x0d  /* 13 */)
#define MK_LH     (0x0e  /* 14 */)
#define MK_SH     (0x0f  /* 15 */)
#define MK_LW     (0x10  /* 16 */)
#define MK_SW     (0x11  /* 17 */)
#define MK_INC    (0x12  /* 18 */)
#define MK_DEC    (0x13  /* 19 */)
#define MK_ADD    (0x14  /* 20 */)
#define MK_SUB    (0x15  /* 21 */)
#define MK_NEG    (0x16  /* 22 */)
#define MK_MUL    (0x17  /* 23 */)
#define MK_DIV    (0x18  /* 24 */)
#define MK_MOD    (0x19  /* 25 */)
#define MK_SLL    (0x1a  /* 26 */)
#define MK_SRL    (0x1b  /* 27 */)
#define MK_SRA    (0x1c  /* 28 */)
#define MK_INV    (0x1d  /* 29 */)
#define MK_XOR    (0x1e  /* 30 */)
#define MK_OR     (0x1f  /* 31 */)
#define MK_AND    (0x20  /* 32 */)
#define MK_ORL    (0x21  /* 33 */)
#define MK_ANDL   (0x22  /* 34 */)
#define MK_GT     (0x23  /* 35 */)
#define MK_LT     (0x24  /* 36 */)
#define MK_GTE    (0x25  /* 37 */)
#define MK_LTE    (0x26  /* 38 */)
#define MK_EQ     (0x27  /* 39 */)
#define MK_NE     (0x28  /* 40 */)
#define MK_DROP   (0x29  /* 41 */)
#define MK_DUP    (0x2a  /* 42 */)
#define MK_OVER   (0x2b  /* 43 */)
#define MK_SWAP   (0x2c  /* 44 */)
#define MK_R      (0x2d  /* 45 */)
#define MK_MTR    (0x2e  /* 46 */)
#define MK_RDROP  (0x2f  /* 47 */)
#define MK_EMIT   (0x30  /* 48 */)
#define MK_PRINT  (0x31  /* 49 */)
#define MK_CR     (0x32  /* 50 */)
#define MK_DOT    (0x33  /* 51 */)
#define MK_DOTH   (0x34  /* 52 */)
#define MK_DOTS   (0x35  /* 53 */)
#define MK_DOTSH  (0x36  /* 54 */)
#define MK_DOTRH  (0x37  /* 55 */)
#define MK_DUMP   (0x38  /* 56 */)

#endif /* LIBMKB_AUTOGEN_H */