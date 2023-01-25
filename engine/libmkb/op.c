// Copyright (c) 2023 Sam Blenny
// SPDX-License-Identifier: MIT
//
#ifndef LIBMKB_OP_C
#define LIBMKB_OP_C

#include "autogen.h"
#include "op.h"

/*
 * These functions implement stack machine CPU opcodes for the Markab VM.
 */

// NOP  ( -- )
static void op_NOP(mk_context_t * ctx) {
}

// RESET  ( -- )
static void op_RESET(mk_context_t * ctx) {
}

// JMP  ( -- )
static void op_JMP(mk_context_t * ctx) {
}

// JAL  ( -- )
static void op_JAL(mk_context_t * ctx) {
}

// RET  ( -- )
static void op_RET(mk_context_t * ctx) {
}

// BZ  ( -- )
static void op_BZ(mk_context_t * ctx) {
}

// BFOR  ( -- )
static void op_BFOR(mk_context_t * ctx) {
}

// U8  ( -- )
static void op_U8(mk_context_t * ctx) {
}

// U16  ( -- )
static void op_U16(mk_context_t * ctx) {
}

// I32  ( -- )
static void op_I32(mk_context_t * ctx) {
}

// HALT  ( -- ) Halt the virtual CPU
static void op_HALT(mk_context_t * ctx) {
	ctx->halted = 1;
}

// TRON  ( -- )
static void op_TRON(mk_context_t * ctx) {
}

// TROFF  ( -- )
static void op_TROFF(mk_context_t * ctx) {
}

// IODUMP  ( -- )
static void op_IODUMP(mk_context_t * ctx) {
}

// IOKEY  ( -- )
static void op_IOKEY(mk_context_t * ctx) {
}

// IORH  ( -- )
static void op_IORH(mk_context_t * ctx) {
}

// IOLOAD  ( -- )
static void op_IOLOAD(mk_context_t * ctx) {
}

// FOPEN  ( -- )
static void op_FOPEN(mk_context_t * ctx) {
}

// FREAD  ( -- )
static void op_FREAD(mk_context_t * ctx) {
}

// FWRITE  ( -- )
static void op_FWRITE(mk_context_t * ctx) {
}

// FSEEK  ( -- )
static void op_FSEEK(mk_context_t * ctx) {
}

// FTELL  ( -- )
static void op_FTELL(mk_context_t * ctx) {
}

// FTRUNC  ( -- )
static void op_FTRUNC(mk_context_t * ctx) {
}

// FCLOSE  ( -- )
static void op_FCLOSE(mk_context_t * ctx) {
}

// MTR  ( -- )
static void op_MTR(mk_context_t * ctx) {
}

// R  ( -- )
static void op_R(mk_context_t * ctx) {
}

// CALL  ( -- )
static void op_CALL(mk_context_t * ctx) {
}

// PC  ( -- )
static void op_PC(mk_context_t * ctx) {
}

// MTE  ( -- )
static void op_MTE(mk_context_t * ctx) {
}

// LB  ( -- )
static void op_LB(mk_context_t * ctx) {
}

// SB  ( -- )
static void op_SB(mk_context_t * ctx) {
}

// LH  ( -- )
static void op_LH(mk_context_t * ctx) {
}

// SH  ( -- )
static void op_SH(mk_context_t * ctx) {
}

// LW  ( -- )
static void op_LW(mk_context_t * ctx) {
}

// SW  ( -- )
static void op_SW(mk_context_t * ctx) {
}

// ADD  ( -- )
static void op_ADD(mk_context_t * ctx) {
}

// SUB  ( -- )
static void op_SUB(mk_context_t * ctx) {
}

// MUL  ( -- )
static void op_MUL(mk_context_t * ctx) {
}

// DIV  ( -- )
static void op_DIV(mk_context_t * ctx) {
}

// MOD  ( -- )
static void op_MOD(mk_context_t * ctx) {
}

// SLL  ( -- )
static void op_SLL(mk_context_t * ctx) {
}

// SRL  ( -- )
static void op_SRL(mk_context_t * ctx) {
}

// SRA  ( -- )
static void op_SRA(mk_context_t * ctx) {
}

// INV  ( -- )
static void op_INV(mk_context_t * ctx) {
}

// XOR  ( -- )
static void op_XOR(mk_context_t * ctx) {
}

// OR  ( -- )
static void op_OR(mk_context_t * ctx) {
}

// AND  ( -- )
static void op_AND(mk_context_t * ctx) {
}

// GT  ( -- )
static void op_GT(mk_context_t * ctx) {
}

// LT  ( -- )
static void op_LT(mk_context_t * ctx) {
}

// EQ  ( -- )
static void op_EQ(mk_context_t * ctx) {
}

// NE  ( -- )
static void op_NE(mk_context_t * ctx) {
}

// ZE  ( -- )
static void op_ZE(mk_context_t * ctx) {
}

// INC  ( -- )
static void op_INC(mk_context_t * ctx) {
}

// DEC  ( -- )
static void op_DEC(mk_context_t * ctx) {
}

// IOEMIT  ( -- )
static void op_IOEMIT(mk_context_t * ctx) {
}

// IODOT  ( -- )
static void op_IODOT(mk_context_t * ctx) {
}

// IODH  ( -- )
static void op_IODH(mk_context_t * ctx) {
}

// IOD  ( -- )
static void op_IOD(mk_context_t * ctx) {
}

// RDROP  ( -- )
static void op_RDROP(mk_context_t * ctx) {
}

// DROP  ( -- )
static void op_DROP(mk_context_t * ctx) {
}

// DUP  ( -- )
static void op_DUP(mk_context_t * ctx) {
}

// OVER  ( -- )
static void op_OVER(mk_context_t * ctx) {
}

// SWAP  ( -- )
static void op_SWAP(mk_context_t * ctx) {
}

// MTA  ( -- )
static void op_MTA(mk_context_t * ctx) {
}

// LBA  ( -- )
static void op_LBA(mk_context_t * ctx) {
}

// LBAI  ( -- )
static void op_LBAI(mk_context_t * ctx) {
}

// AINC  ( -- )
static void op_AINC(mk_context_t * ctx) {
}

// ADEC  ( -- )
static void op_ADEC(mk_context_t * ctx) {
}

// A  ( -- )
static void op_A(mk_context_t * ctx) {
}

// MTB  ( -- )
static void op_MTB(mk_context_t * ctx) {
}

// LBB  ( -- )
static void op_LBB(mk_context_t * ctx) {
}

// LBBI  ( -- )
static void op_LBBI(mk_context_t * ctx) {
}

// SBBI  ( -- )
static void op_SBBI(mk_context_t * ctx) {
}

// BINC  ( -- )
static void op_BINC(mk_context_t * ctx) {
}

// BDEC  ( -- )
static void op_BDEC(mk_context_t * ctx) {
}

// B  ( -- )
static void op_B(mk_context_t * ctx) {
}

// TRUE  ( -- )
static void op_TRUE(mk_context_t * ctx) {
}

// FALSE  ( -- )
static void op_FALSE(mk_context_t * ctx) {
}


#endif /* LIBMKB_OP_C */
