/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * This file implements opcodes for the bytecode interpreter of the Markab VM's
 * stack machine CPU.
 */
#ifndef LIBMKB_OP_C
#define LIBMKB_OP_C

#include "autogen.h"
#include "op.h"

/* ============================================================================
 * These macros reduce repetition of boilerplate code in opcode functions:
 * ============================================================================
 */

/* Macro for asserting minimum data stack depth. If the assertion fails, this
 * will raise a VM error interrupt and cause the enclosing function to return.
 * Enclosing function must declare `mk_context_t * ctx`.
 */
#define _assert_data_stack_depth_is_at_least(N) \
    if(ctx->DSDeep < N) {                       \
        vm_irq_err(ctx, MK_ERR_D_UNDER);        \
        return;                                 \
    }

/* Macro for asserting maximum data stack depth. If the assertion fails, this
 * will raise a VM error interrupt and cause the enclosing function to return.
 * Enclosing function must declare `mk_context_t * ctx`.
 */
#define _assert_data_stack_is_not_full() \
    if(ctx->DSDeep > 17) {               \
        vm_irq_err(ctx, MK_ERR_D_OVER);  \
        return;                          \
    }

/* Macro for asserting minimum return stack depth. If the assertion fails, this
 * will raise a VM error interrupt, reset the stacks, and cause the enclosing
 * function to return. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _assert_return_stack_depth_is_at_least(N) \
    if(ctx->DSDeep < N) {                         \
        op_RESET(ctx);                            \
        vm_irq_err(ctx, MK_ERR_D_UNDER);          \
        return;                                   \
    }

/* Macro for asserting maximum return stack depth. If the assertion fails, this
 * will raise a VM error interrupt, reset the stacks, cause the enclosing
 * function to return. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _assert_return_stack_is_not_full() \
    if(ctx->RSDeep > 16) {                 \
        op_RESET(ctx);                     \
        vm_irq_err(ctx, MK_ERR_R_OVER);    \
        return;                            \
    }

/* Macro to nip (discard) S, second item of data stack, without checking stack
 * depth. This is for macros or opcode functions that have already checked the
 * stack depth. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _nip_S_without_minimum_stack_depth_check() \
    if(ctx->DSDeep > 2) {                          \
        u8 thirdOnStack = ctx->DSDeep - 3;         \
        ctx->S = ctx->DStack[thirdOnStack];        \
    }                                              \
    ctx->DSDeep -= 1;

/* Macro to drop T, top item of the data stack, without checking stack depth.
 * This is for macros or opcode functions that have already checked the stack
 * depth. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _drop_T_without_minimum_stack_depth_check() \
    ctx->T = ctx->S;                                \
    _nip_S_without_minimum_stack_depth_check()

/* Macro to drop R, top item of the return stack, without checking stack depth.
 * This is for macros or opcode functions that have already checked the stack
 * depth. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _rdrop_R_without_minimum_stack_depth_check()                 \
    if(ctx->RSDeep > 1) {                                            \
        ctx->R = ctx->RStack[ctx->RSDeep - 2 /* second on stack */]; \
    }                                                                \
    ctx->RSDeep -= 1;

/* Macro to apply N=λ(S,T), storing the result in T and nipping S.
 * Enclosing function must declare an instance of `mk_context_t * ctx`.
 */
#define _apply_lambda_ST(N)                    \
    _assert_data_stack_depth_is_at_least(2)    \
    ctx->T = (N);                              \
    _nip_S_without_minimum_stack_depth_check()

/* Macro to apply N=λ(T), storing the result in T.
 * Enclosing function must declare an instance of `mk_context_t * ctx`.
 */
#define _apply_lambda_T(N)                  \
    _assert_data_stack_depth_is_at_least(1) \
    ctx->T = (N);

/* Macro to push N onto the data stack as a 32-bit signed integer, without
 * checking maximum stack depth. This is for macros or opcode functions that
 * have already checked the stack depth. Enclosing function must declare
 * `mk_context_t * ctx`.
 */
#define _push_T_without_max_stack_depth_check(N)                    \
    if(ctx->DSDeep > 1) {                                           \
        ctx->DStack[ctx->DSDeep - 2 /* third on stack */] = ctx->S; \
    }                                                               \
    ctx->S = ctx->T;                                                \
    ctx->T = (N);                                                   \
    ctx->DSDeep += 1;

/* Macro to push N onto the return stack as a 32-bit signed integer, without
 * checking maximum stack depth. This is for macros or opcode functions that
 * have already checked the stack depth. Enclosing function must declare
 * `mk_context_t * ctx`.
 */
#define _rpush_R_without_max_stack_depth_check(N)                    \
    if(ctx->RSDeep > 0) {                                            \
        ctx->RStack[ctx->RSDeep - 1 /* second on stack */] = ctx->R; \
    }                                                                \
    ctx->R = (N);                                                    \
    ctx->RSDeep += 1;


/* ============================================================================
 * These are opcode implementations:
 * ============================================================================
 */

/* NOP ( -- ) Spend one virtual CPU clock cycle doing nothing. */
static void op_NOP(mk_context_t * ctx) {
    /* Do nothing. On purpose. */
}

/* RESET ( -- ) Reset data stack, return stack, error code, and input buffer.
 */
static void op_RESET(mk_context_t * ctx) {
    ctx->DSDeep = 0;
    ctx->RSDeep = 0;
    ctx->err = 0;
    int i; /* declare outside of for loop for ANSI C compatibility */
    for(i=0; i<MK_BufMax; i++) {
        ctx->InBuf[i] = 0;
    }
}

/* JMP ( -- ) Jump to subroutine at address read from instruction stream.
 *     The jump address is PC-relative to allow for relocatable object code.
 */
static void op_JMP(mk_context_t * ctx) {
    u16 pc = ctx->PC;
    u16 n = (ctx->RAM[pc+1] << 8) | ctx->RAM[pc];  /* signed LE halfword */
    /* Add offset to program counter to compute destination address.
     * CAUTION! CAUTION! CAUTION!
     * This is relying on u16 (uint16_t) integer overflow behavior to give
     * results modulo 0xffff to let us do math like (5-100) = 65441. This lets
     * signed 16-bit pc-relative offsets address the full memory range.
     */
    ctx->PC += n;  /* CAUTION! This is relying on an implicit modulo 0xffff */
}

/* JAL ( -- ) Jump to subroutine after pushing old value of PC to return stack.
 *     The jump address is PC-relative to allow for relocatable object code.
 */
static void op_JAL(mk_context_t * ctx) {
    _assert_return_stack_is_not_full()
    /* Read a 16-bit signed offset (relative to PC) from instruction stream */
    u16 pc = ctx->PC;
    u16 n = (ctx->RAM[pc+1] << 8) + ctx->RAM[pc];
    /* Push the current Program Counter (PC) to return stack */
    _rpush_R_without_max_stack_depth_check(pc + 2)
    /* Add offset to program counter to compute destination address.
     * CAUTION! CAUTION! CAUTION!
     * This is relying on u16 (uint16_t) integer overflow behavior to give
     * results modulo 0xffff to let us do math like (5-100) = 65441. This lets
     * signed 16-bit pc-relative offsets address the full memory range.
     */
    ctx->PC = pc + n;
}

/* RET ( -- ) Return from subroutine, taking address from return stack. */
static void op_RET(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1)
    /* Set program counter from top of return stack */
    ctx->PC = (u8) ctx->R;
    /* Drop R */
    _rdrop_R_without_minimum_stack_depth_check()
}

/* BZ ( T -- ) Branch to PC-relative address if T == 0, drop T.
 *    The branch address is PC-relative to allow for relocatable object code.
 */
static void op_BZ(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1)
    u8 pc = ctx->PC;
    if(ctx->T == 0) {
        /* Branch forward past conditional block: Add address literal from */
        /* instruction stream to PC. Maximum branch distance is +255. */
        ctx->PC = pc + ctx->RAM[pc];
    } else {
        /* Enter conditional block: Advance PC past address literal */
        ctx->PC = pc + 1;
    }
    /* Drop T the fast way */
    _drop_T_without_minimum_stack_depth_check()
}

/* BFOR ( -- ) */
static void op_BFOR(mk_context_t * ctx) {
}

/* U8 ( -- ) */
static void op_U8(mk_context_t * ctx) {
}

/* U16 ( -- ) */
static void op_U16(mk_context_t * ctx) {
}

/* I32 ( -- ) */
static void op_I32(mk_context_t * ctx) {
}

/* HALT ( -- ) Halt the virtual CPU */
static void op_HALT(mk_context_t * ctx) {
    ctx->halted = 1;
}

/* TRON ( -- ) */
static void op_TRON(mk_context_t * ctx) {
}

/* TROFF ( -- ) */
static void op_TROFF(mk_context_t * ctx) {
}

/* IODUMP ( -- ) */
static void op_IODUMP(mk_context_t * ctx) {
}

/* IOKEY ( -- ) */
static void op_IOKEY(mk_context_t * ctx) {
}

/* IORH ( -- ) */
static void op_IORH(mk_context_t * ctx) {
}

/* IOLOAD ( -- ) */
static void op_IOLOAD(mk_context_t * ctx) {
}

/* FOPEN ( -- ) */
static void op_FOPEN(mk_context_t * ctx) {
}

/* FREAD ( -- ) */
static void op_FREAD(mk_context_t * ctx) {
}

/* FWRITE ( -- ) */
static void op_FWRITE(mk_context_t * ctx) {
}

/* FSEEK ( -- ) */
static void op_FSEEK(mk_context_t * ctx) {
}

/* FTELL ( -- ) */
static void op_FTELL(mk_context_t * ctx) {
}

/* FTRUNC ( -- ) */
static void op_FTRUNC(mk_context_t * ctx) {
}

/* FCLOSE ( -- ) */
static void op_FCLOSE(mk_context_t * ctx) {
}

/* MTR ( T -- ) Move T to R. */
static void op_MTR(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1)
    _assert_return_stack_is_not_full()
    _rpush_R_without_max_stack_depth_check(ctx->T)
    _drop_T_without_minimum_stack_depth_check()
}

/* R ( -- r ) Push a copy of the top of the return stack (R) as T. */
static void op_R(mk_context_t * ctx) {
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(ctx->R)
}

/* CALL ( -- ) */
static void op_CALL(mk_context_t * ctx) {
}

/* PC ( -- pc ) Push value of program counter (PC) register as T. */
static void op_PC(mk_context_t * ctx) {
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(ctx->PC)
}

/* MTE ( err -- ) Move value from T into the ERR register (raise an error). */
static void op_MTE(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1)
    ctx->err = ctx->T;
    _drop_T_without_minimum_stack_depth_check()
}

/* LB ( addr -- i8 ) Load i8 (signed byte) at address T into T as an i32. */
static void op_LB(mk_context_t * ctx) {
}

/* SB ( u8 addr -- ) Store u8 (unsigned byte) from S into address T. */
static void op_SB(mk_context_t * ctx) {
}

/* LH ( addr -- i32 ) Load i16 (signed halfword) at address T into T as an i32.
 */
static void op_LH(mk_context_t * ctx) {
}

/* SH ( i16 addr -- ) Store u16 (unsigned halfword) from S into address T. */
static void op_SH(mk_context_t * ctx) {
}

/* LW ( addr -- i32 ) Load i32 (signed word) at address T into T. */
static void op_LW(mk_context_t * ctx) {
}

/* SW ( u32 addr -- ) Store u32 (unsigned word) from S into address T. */
static void op_SW(mk_context_t * ctx) {
}

/* ADD ( S T -- S+T ) Store S+T in T, nip S. */
static void op_ADD(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S + ctx->T)
}

/* SUB ( S T -- S-T ) Store S-T in T, nip S. */
static void op_SUB(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S - ctx->T)
}

/* MUL ( S T -- S*T ) Store S*T in T, nip S. */
static void op_MUL(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S * ctx->T)
}

/* DIV ( S T -- S/T ) Store S/T in T, nip S. (CAUTION! integer division) */
static void op_DIV(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S / ctx->T)
}

/* MOD ( S T -- S%T ) Store S modulo T in T, nip S. */
static void op_MOD(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S % ctx->T)
}

/* SLL ( S T -- S<<T ) Store S logical left-shifted by T bits in T, nip S. */
static void op_SLL(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S << ctx->T)
}

/* SRL ( S T -- S>>T ) Store S logical right-shifted by T bits in T, nip S.
 *     This is the shift to use if you want zero-fill on the left.
 */
static void op_SRL(mk_context_t * ctx) {
    _apply_lambda_ST(((u32)ctx->S) >> ctx->T)
}

/* SRA ( S T -- S>>>T ) Store S arithmetic-shifted by T bits in T, nip S.
 *     This is the shift to use if you want sign-bit-fill on the left.
 *     CAUTION! This seems to be a murky area of the C spec. Possible UB.
 */
static void op_SRA(mk_context_t * ctx) {
    _apply_lambda_ST(((i32)ctx->S) >> ctx->T)
}

/* INV ( n -- ~n ) Do a bitwise inversion of each bit in T. */
static void op_INV(mk_context_t * ctx) {
    _apply_lambda_T(~ (ctx->T))
}

/* XOR ( S T -- S^T ) Calculate S bitwise_XOR T. */
static void op_XOR(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S ^ ctx->T)
}

/* OR ( S T -- S|T ) Calculate S bitwise_OR T, which, in Markab, is also a
 *     logical OR because we're using Forth-style truth values.
 */
static void op_OR(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S | ctx->T)
}

/* AND ( S T -- S&T ) Calculate S bitwise_AND T, which, in Markab, is also a
 *     logical AND because we're using Forth-style truth values.
 */
static void op_AND(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S & ctx->T)
}

/* GT ( S T -- S>T ) Test S>T with Forth-style truth value (true is -1). */
static void op_GT(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S > ctx->T ? -1 : 0)
}

/* LT ( S T -- S<T ) Test S<T with Forth-style truth value (true is -1). */
static void op_LT(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S < ctx->T ? -1 : 0)
}

/* EQ ( S T -- S==T ) Test S==T with Forth-style truth value (true is -1). */
static void op_EQ(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S != ctx->T ? -1 : 0)
}

/* NE ( S T -- S!=T ) Test S!=T with Forth-style truth value (true is -1). */
static void op_NE(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S != ctx->T ? -1 : 0)
}

/* ZE ( n -- n==0 ) Test T==0 with Forth-style truth value (true is -1). */
static void op_ZE(mk_context_t * ctx) {
    _apply_lambda_T(ctx->T == 0 ? -1 : 0)
}

/* INC ( n -- n+1 ) Increment the value in T. */
static void op_INC(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1)
    ctx->T += 1;
}

/* DEC ( n -- n-1 ) Decrement the value in T. */
static void op_DEC(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1)
    ctx->T -= 1;
}

/* IOEMIT ( u8 -- ) */
static void op_IOEMIT(mk_context_t * ctx) {
}

/* IODOT ( i32 -- ) */
static void op_IODOT(mk_context_t * ctx) {
}

/* IODH ( -- ) */
static void op_IODH(mk_context_t * ctx) {
}

/* IOD ( -- ) */
static void op_IOD(mk_context_t * ctx) {
}

/* RDROP ( -- ) Drop R, the top item of the return stack. */
static void op_RDROP(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1)
    _rdrop_R_without_minimum_stack_depth_check()
}

/* DROP ( n -- ) Drop T, the top item of the data stack. */
static void op_DROP(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1)
    _drop_T_without_minimum_stack_depth_check()
}

/* DUP ( n1 -- n1 n1 ) Duplicate Top item of data stack (push a copy of T). */
static void op_DUP(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1)
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(ctx->T)
}

/* OVER ( n1 n2 -- n1 n2 n1 ) Push a copy of Second data stack item. */
static void op_OVER(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2)
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(ctx->S)
}

/* SWAP ( n1 n2 -- n2 n1 ) Swap the Second and Top items on the data stack. */
static void op_SWAP(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2)
    i32 n = ctx->T;
    ctx->T = ctx->S;
    ctx->S = n;
}

/* MTA ( T -- ) Move the value of T into the A register. */
static void op_MTA(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1)
    ctx->A = ctx->T;
    _drop_T_without_minimum_stack_depth_check()
}

/* LBA ( -- ) */
static void op_LBA(mk_context_t * ctx) {
}

/* LBAI ( -- ) */
static void op_LBAI(mk_context_t * ctx) {
}

/* AINC ( -- ) Increment the value of the A register. */
static void op_AINC(mk_context_t * ctx) {
    ctx->A += 1;
}

/* ADEC ( -- ) Decrement the value of the A register. */
static void op_ADEC(mk_context_t * ctx) {
    ctx->A -= 1;
}

/* A ( -- a ) Push the value of the A register onto the data stack. */
static void op_A(mk_context_t * ctx) {
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(ctx->A)
}

/* MTB ( T -- ) Move the value of T into the B register. */
static void op_MTB(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1)
    ctx->B = ctx->T;
    _drop_T_without_minimum_stack_depth_check()
}

/* LBB ( -- ) */
static void op_LBB(mk_context_t * ctx) {
}

/* LBBI ( -- ) */
static void op_LBBI(mk_context_t * ctx) {
}

/* SBBI ( -- ) */
static void op_SBBI(mk_context_t * ctx) {
}

/* BINC ( -- ) Increment the value of the B register. */
static void op_BINC(mk_context_t * ctx) {
    ctx->B += 1;
}

/* BDEC ( -- ) Decrement the value of the B register. */
static void op_BDEC(mk_context_t * ctx) {
    ctx->B -= 1;
}

/* B ( -- b ) Push the value of the B register onto the data stack. */
static void op_B(mk_context_t * ctx) {
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(ctx->B)
}

/* TRUE ( -- ) Push the Forth-style truth value, which is -1 (all bits set). */
static void op_TRUE(mk_context_t * ctx) {
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(-1)
}

/* FALSE ( -- ) Push the Forth-style false value, which is 0 (all bits clear).
 */
static void op_FALSE(mk_context_t * ctx) {
    _assert_data_stack_is_not_full()
    _push_T_without_max_stack_depth_check(0)
}


#endif /* LIBMKB_OP_C */
