/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * These functions implement stack machine CPU opcodes for the Markab VM.
 */
#ifndef LIBMKB_OP_H
#define LIBMKB_OP_H

#ifdef PLAN_9
#  include "libmkb/autogen.h"
#else
#  include "autogen.h"
#endif

static void op_NOP(mk_context_t * ctx);

static void op_RESET(mk_context_t * ctx);

static void op_JMP(mk_context_t * ctx);

static void op_JAL(mk_context_t * ctx);

static void op_RET(mk_context_t * ctx);

static void op_BZ(mk_context_t * ctx);

static void op_BFOR(mk_context_t * ctx);

static void op_U8(mk_context_t * ctx);

static void op_U16(mk_context_t * ctx);

static void op_I32(mk_context_t * ctx);

static void op_HALT(mk_context_t * ctx);

static void op_TRON(mk_context_t * ctx);

static void op_TROFF(mk_context_t * ctx);

static void op_IODUMP(mk_context_t * ctx);

static void op_IOKEY(mk_context_t * ctx);

static void op_IORH(mk_context_t * ctx);

static void op_IOLOAD(mk_context_t * ctx);

static void op_FOPEN(mk_context_t * ctx);

static void op_FREAD(mk_context_t * ctx);

static void op_FWRITE(mk_context_t * ctx);

static void op_FSEEK(mk_context_t * ctx);

static void op_FTELL(mk_context_t * ctx);

static void op_FTRUNC(mk_context_t * ctx);

static void op_FCLOSE(mk_context_t * ctx);

static void op_MTR(mk_context_t * ctx);

static void op_R(mk_context_t * ctx);

static void op_CALL(mk_context_t * ctx);

static void op_PC(mk_context_t * ctx);

static void op_MTE(mk_context_t * ctx);

static void op_LB(mk_context_t * ctx);

static void op_SB(mk_context_t * ctx);

static void op_LH(mk_context_t * ctx);

static void op_SH(mk_context_t * ctx);

static void op_LW(mk_context_t * ctx);

static void op_SW(mk_context_t * ctx);

static void op_ADD(mk_context_t * ctx);

static void op_SUB(mk_context_t * ctx);

static void op_MUL(mk_context_t * ctx);

static void op_DIV(mk_context_t * ctx);

static void op_MOD(mk_context_t * ctx);

static void op_SLL(mk_context_t * ctx);

static void op_SRL(mk_context_t * ctx);

static void op_SRA(mk_context_t * ctx);

static void op_INV(mk_context_t * ctx);

static void op_XOR(mk_context_t * ctx);

static void op_OR(mk_context_t * ctx);

static void op_AND(mk_context_t * ctx);

static void op_GT(mk_context_t * ctx);

static void op_LT(mk_context_t * ctx);

static void op_EQ(mk_context_t * ctx);

static void op_NE(mk_context_t * ctx);

static void op_ZE(mk_context_t * ctx);

static void op_INC(mk_context_t * ctx);

static void op_DEC(mk_context_t * ctx);

static void op_IOEMIT(mk_context_t * ctx);

static void op_IODOT(mk_context_t * ctx);

static void op_IODH(mk_context_t * ctx);

static void op_IOD(mk_context_t * ctx);

static void op_RDROP(mk_context_t * ctx);

static void op_DROP(mk_context_t * ctx);

static void op_DUP(mk_context_t * ctx);

static void op_OVER(mk_context_t * ctx);

static void op_SWAP(mk_context_t * ctx);

static void op_MTA(mk_context_t * ctx);

static void op_LBA(mk_context_t * ctx);

static void op_LBAI(mk_context_t * ctx);

static void op_AINC(mk_context_t * ctx);

static void op_ADEC(mk_context_t * ctx);

static void op_A(mk_context_t * ctx);

static void op_MTB(mk_context_t * ctx);

static void op_LBB(mk_context_t * ctx);

static void op_LBBI(mk_context_t * ctx);

static void op_SBBI(mk_context_t * ctx);

static void op_BINC(mk_context_t * ctx);

static void op_BDEC(mk_context_t * ctx);

static void op_B(mk_context_t * ctx);

static void op_TRUE(mk_context_t * ctx);

static void op_FALSE(mk_context_t * ctx);

#endif /* LIBMKB_OP_H */
