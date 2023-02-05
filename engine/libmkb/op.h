/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * These functions implement stack machine CPU opcodes for the Markab VM.
 */
#ifndef LIBMKB_OP_H
#define LIBMKB_OP_H

/* NOP */
static void op_NOP(void);

/* VM Control */
static void op_RESET(mk_context_t * ctx);
static void op_HALT(mk_context_t * ctx);
static void op_TRON(mk_context_t * ctx);
static void op_TROFF(mk_context_t * ctx);
static void op_MTE(mk_context_t * ctx);

/* Literals */
static void op_U8(mk_context_t * ctx);
static void op_U16(mk_context_t * ctx);
static void op_I32(mk_context_t * ctx);
static void op_STR(mk_context_t * ctx);

/* Branch, Jump, Call, Return */
static void op_BZ(mk_context_t * ctx);
static void op_JMP(mk_context_t * ctx);
static void op_JAL(mk_context_t * ctx);
static void op_RET(mk_context_t * ctx);
static void op_CALL(mk_context_t * ctx);

/* Memory Access: Loads and Stores */
static void op_LB(mk_context_t * ctx);
static void op_SB(mk_context_t * ctx);
static void op_LH(mk_context_t * ctx);
static void op_SH(mk_context_t * ctx);
static void op_LW(mk_context_t * ctx);
static void op_SW(mk_context_t * ctx);

/* Arithmetic */
static void op_INC(mk_context_t * ctx);
static void op_DEC(mk_context_t * ctx);
static void op_ADD(mk_context_t * ctx);
static void op_SUB(mk_context_t * ctx);
static void op_MUL(mk_context_t * ctx);
static void op_DIV(mk_context_t * ctx);
static void op_MOD(mk_context_t * ctx);

/* Shifts */
static void op_SLL(mk_context_t * ctx);
static void op_SRL(mk_context_t * ctx);
static void op_SRA(mk_context_t * ctx);

/* Locic Operations */
static void op_INV(mk_context_t * ctx);
static void op_XOR(mk_context_t * ctx);
static void op_OR(mk_context_t * ctx);
static void op_AND(mk_context_t * ctx);

/* Comparisons */
static void op_GT(mk_context_t * ctx);
static void op_LT(mk_context_t * ctx);
static void op_EQ(mk_context_t * ctx);
static void op_NE(mk_context_t * ctx);
static void op_ZE(mk_context_t * ctx);

/* Truth Value Constants */
static void op_TRUE(mk_context_t * ctx);
static void op_FALSE(mk_context_t * ctx);

/* Data Stack Operations */
static void op_DROP(mk_context_t * ctx);
static void op_DUP(mk_context_t * ctx);
static void op_OVER(mk_context_t * ctx);
static void op_SWAP(mk_context_t * ctx);
static void op_PC(mk_context_t * ctx);

/* Return Stack Operations */
static void op_R(mk_context_t * ctx);
static void op_MTR(mk_context_t * ctx);
static void op_RDROP(mk_context_t * ctx);

/* Console IO */
static void op_EMIT(mk_context_t * ctx);
static void op_HEX(mk_context_t * ctx);
static void op_DECIMAL(mk_context_t * ctx);
static void op_BASE(mk_context_t * ctx);
static void op_PRINT(mk_context_t * ctx);

/* Debug Dumps for Stacks and Memory */
static void op_DOT(mk_context_t * ctx);
static void op_DOTS(mk_context_t * ctx);
static void op_DOTSH(mk_context_t * ctx);
static void op_DOTRH(mk_context_t * ctx);
static void op_DUMP(mk_context_t * ctx);

#endif /* LIBMKB_OP_H */
