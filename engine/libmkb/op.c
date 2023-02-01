/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * This file implements opcodes for the bytecode interpreter of the Markab VM's
 * stack machine CPU.
 */
#ifndef LIBMKB_OP_C
#define LIBMKB_OP_C

#include "libmkb.h"
#include "autogen.h"
#include "fmt.h"
#include "op.h"
#include "vm.h"

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
        vm_irq_err(MK_ERR_D_UNDER);             \
        return;                                 \
    }

/* Macro for asserting maximum data stack depth. If the assertion fails, this
 * will raise a VM error interrupt and cause the enclosing function to return.
 * Enclosing function must declare `mk_context_t * ctx`.
 */
#define _assert_data_stack_is_not_full() \
    if(ctx->DSDeep > 17) {               \
        vm_irq_err(MK_ERR_D_OVER);       \
        return;                          \
    }

/* Macro for asserting minimum return stack depth. If the assertion fails, this
 * will raise a VM error interrupt, reset the stacks, and cause the enclosing
 * function to return. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _assert_return_stack_depth_is_at_least(N) \
    if(ctx->DSDeep < N) {                         \
        op_RESET(ctx);                            \
        vm_irq_err(MK_ERR_D_UNDER);               \
        return;                                   \
    }

/* Macro for asserting maximum return stack depth. If the assertion fails, this
 * will raise a VM error interrupt, reset the stacks, cause the enclosing
 * function to return. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _assert_return_stack_is_not_full() \
    if(ctx->RSDeep > 16) {                 \
        op_RESET(ctx);                     \
        vm_irq_err(MK_ERR_R_OVER);         \
        return;                            \
    }

/* Macro to nip (discard) S, second item of data stack, without checking stack
 * depth. This is for macros or opcode functions that have already checked the
 * stack depth. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _nip_S_without_minimum_stack_depth_check() { \
    if(ctx->DSDeep > 2) {                            \
        u8 thirdOnStack = ctx->DSDeep - 3;           \
        ctx->S = ctx->DStack[thirdOnStack];          \
    }                                                \
    ctx->DSDeep -= 1;                                }

/* Macro to drop T, top item of the data stack, without checking stack depth.
 * This is for macros or opcode functions that have already checked the stack
 * depth. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _drop_T() {                             \
    ctx->T = ctx->S;                            \
    _nip_S_without_minimum_stack_depth_check(); }

/* Macro to drop top 2 items of data stack without checking stack depth.
 * This is for macros or opcode functions that have already checked the stack
 * depth. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _drop_S_and_T() {                      \
    if(ctx->DSDeep > 3) {                      \
        ctx->T = ctx->DStack[ctx->DSDeep - 3]; \
        ctx->S = ctx->DStack[ctx->DSDeep - 4]; \
    } else if(ctx->DSDeep > 2) {               \
        ctx->T = ctx->DStack[ctx->DSDeep - 3]; \
    }                                          \
    ctx->DSDeep -= 2;                          }

/* Macro to drop R, top item of the return stack, without checking stack depth.
 * This is for macros or opcode functions that have already checked the stack
 * depth. Enclosing function must declare `mk_context_t * ctx`.
 */
#define _drop_R() {                                                  \
    if(ctx->RSDeep > 1) {                                            \
        ctx->R = ctx->RStack[ctx->RSDeep - 2 /* second on stack */]; \
    }                                                                \
    ctx->RSDeep -= 1;                                                }

/* Macro to apply N=λ(S,T), storing the result in T and nipping S.
 * Enclosing function must declare an instance of `mk_context_t * ctx`.
 */
#define _apply_lambda_ST(N) {                   \
    _assert_data_stack_depth_is_at_least(2);    \
    ctx->T = (N);                               \
    _nip_S_without_minimum_stack_depth_check(); }

/* Macro to apply N=λ(T), storing the result in T.
 * Enclosing function must declare an instance of `mk_context_t * ctx`.
 */
#define _apply_lambda_T(N) {                 \
    _assert_data_stack_depth_is_at_least(1); \
    ctx->T = (N);                            }

/* Macro to push N onto the data stack as a 32-bit signed integer, without
 * checking maximum stack depth. This is for macros or opcode functions that
 * have already checked the stack depth. Enclosing function must declare
 * `mk_context_t * ctx`.
 */
#define _push_T(N) {                                                \
    if(ctx->DSDeep > 1) {                                           \
        ctx->DStack[ctx->DSDeep - 2 /* third on stack */] = ctx->S; \
    }                                                               \
    ctx->S = ctx->T;                                                \
    ctx->T = (N);                                                   \
    ctx->DSDeep += 1;                                               }

/* Macro to push N onto the return stack as a 32-bit signed integer, without
 * checking maximum stack depth. This is for macros or opcode functions that
 * have already checked the stack depth. Enclosing function must declare
 * `mk_context_t * ctx`.
 */
#define _push_R(N) {                                                 \
    if(ctx->RSDeep > 0) {                                            \
        ctx->RStack[ctx->RSDeep - 1 /* second on stack */] = ctx->R; \
    }                                                                \
    ctx->R = (N);                                                    \
    ctx->RSDeep += 1;                                                }

/* Macro to assert that ADDR is within the valid range of RAM addresses */
/* CAUTION! This can cause the enclosing function to return. */
#define _assert_valid_address(ADDR)     \
    if((u32)(ADDR) > MK_RamMax) {       \
        vm_irq_err(MK_ERR_BAD_ADDRESS); \
        return;                         \
    }

/* Macro to read u8 (byte) little-endian integer from RAM */
#define _peek_u8(N)  ((u8) ctx->RAM[(u16)(N)])

/* Macro to read u16 (halfword) little-endian integer from RAM */
#define _peek_u16(N)  (                     \
    (((u16) ctx->RAM[(u16)(N) + 1]) << 8) + \
    ( (u16) ctx->RAM[(u16)(N)    ])         )

/* Macro to read u32 (word) little-endian integer from RAM */
#define _peek_u32(N)  (                      \
    (((u32) ctx->RAM[(u16)(N) + 3]) << 24) + \
    (((u32) ctx->RAM[(u16)(N) + 2]) << 16) + \
    (((u32) ctx->RAM[(u16)(N) + 1]) <<  8) + \
    ( (u32) ctx->RAM[(u16)(N)    ])          )

/* Macro to write u8 N into RAM address ADDR
 * CAUTION! The RAM[(u16)(...)] cast here avoids out of range memory access to
 * protect against stack corruption or segfaulting, but it does not provide
 * error detection. Use a separate assertion to handle that part.
 */
#define _poke_u8(N, ADDR) { ctx->RAM[(u16)(ADDR)] = (u8)(N); }

/* Macro to write u16 N to RAM as little-endian integer at address ADDR
 * CAUTION! The RAM[(u16)(...)] casts here avoid out of range memory access to
 * protect against stack corruption or segfaulting, but they do not provide
 * error detection. Use a separate assertion to handle that part.
 */
#define _poke_u16(N, ADDR) {                       \
    ctx->RAM[(u16) (ADDR)     ] = (u8)  (N)      ; \
    ctx->RAM[(u16)((ADDR) + 1)] = (u8) ((N) >> 8); }

/* Macro to write u32 N to RAM as little-endian integer at address ADDR
 * CAUTION! The RAM[(u16)(...)] casts here avoid out of range memory access to
 * protect against stack corruption or segfaulting, but they do not provide
 * error detection. Use a separate assertion to handle that part.
 */
#define _poke_u32(N, ADDR) {                        \
    ctx->RAM[(u16) (ADDR)     ] = (u8)  (N)       ; \
    ctx->RAM[(u16)((ADDR) + 1)] = (u8) ((N) >>  8); \
    ctx->RAM[(u16)((ADDR) + 2)] = (u8) ((N) >> 16); \
    ctx->RAM[(u16)((ADDR) + 3)] = (u8) ((N) >> 24); }

/* Macro to read u8 (byte) literal from instruction stream */
#define _u8_lit()  (_peek_u8(ctx->PC))

/* Macro to read u16 (halfword) literal from instruction stream */
#define _u16_lit()  (_peek_u16(ctx->PC))

/* Macro to read u32 (word) literal from instruction stream */
#define _u32_lit()  (_peek_u32(ctx->PC))

/* Macro to add signed relative offset N to the program counter (PC). The
 * addition is done with an implicit modulo 0xffff to allow addressing into
 * the full 64kB memory space with relative addresses. This is intended to
 * facilitate compiling of relocatable code, which is useful for things like
 * doing a self-hosted compile of the kernel and compiler into a new rom file.
 * NOTE: N can be u8, i8, u16, or i16. The typecast will make it work out.
 */
#define _adjust_PC_by(N)  { ctx->PC = (ctx->PC + (u16)(N)); }


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
    _assert_valid_address(ctx->PC + 1);
    u16 n = _u16_lit();
    /* Add offset to program counter to compute destination address. */
    _adjust_PC_by(n);
}

/* JAL ( -- ) Jump to subroutine after pushing old value of PC to return stack.
 *     The jump address is PC-relative to allow for relocatable object code.
 */
static void op_JAL(mk_context_t * ctx) {
    _assert_return_stack_is_not_full();
    /* Push the current Program Counter (PC) to return stack */
    _push_R(ctx->PC + 2);
    /* Read a 16-bit signed offset (relative to PC) from instruction stream */
    _assert_valid_address(ctx->PC + 1);
    u16 n = _u16_lit();
    /* Change PC to the jump's destination address. */
    _adjust_PC_by(n);
}

/* RET ( -- ) Return from subroutine, taking address from return stack. */
static void op_RET(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1);
    /* Set program counter from top of return stack */
    ctx->PC = ctx->R;
    _drop_R();
}

/* BZ ( T -- ) Branch to PC-relative address if T == 0, drop T.
 *    The branch address is PC-relative to allow for relocatable object code.
 */
static void op_BZ(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    if(ctx->T == 0) {
        /* Branch forward past conditional block: Add address literal from */
        /* instruction stream to PC. Maximum branch distance is +255. */
        _assert_valid_address(ctx->PC);
        u8 n = _u8_lit();
        _adjust_PC_by(n);
    } else {
        /* Enter conditional block: Advance PC past address literal */
        _adjust_PC_by(1);
    }
    _drop_T();
}

/* BFOR ( -- ) Decrement R and branch to start of loop if R > 0, drop R at end
 *     of loop. PC-relative branch address allows for relocatable object code.
 */
static void op_BFOR(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1);
    ctx->R -= 1;
    if(ctx->R > 0) {
        /* Keep looping: Branch backwards by subtracting byte literal from PC
         * Maximum branch distance is -255
         */
        ctx->PC -= _peek_u8(ctx->PC);
    } else {
        /* End of loop: Advance PC past address literal, drop R */
        ctx->PC += 1;
        _drop_R();
    }
}

/* U8 ( -- ) Read u8 byte literal, zero-extend it, push as T. */
static void op_U8(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    /* Read and push an 8-bit unsigned integer from instruction stream */
    _assert_valid_address(ctx->PC);
    i32 zero_extended = (i32) _u8_lit();
    _push_T(zero_extended);
    /* advance program counter past the literal */
    _adjust_PC_by(1);
}

/* U16 ( -- ) Read u16 halfword literal, zero-extend it, push as T. */
static void op_U16(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    /* Read and push 16-bit unsigned integer from instruction stream */
    _assert_valid_address(ctx->PC + 1);
    i32 zero_extended = (i32) _u16_lit();
    _push_T(zero_extended);
    /* advance program counter past literal */
    _adjust_PC_by(2);
}

/* I32 ( -- ) Read i32 word literal, push as T. */
static void op_I32(mk_context_t * ctx) {
    _assert_data_stack_is_not_full()
    /* Read and push 32-bit signed integer from instruction stream */
    _assert_valid_address(ctx->PC + 3);
    i32 n = (i32) _u32_lit();
    _push_T(n)
    /* advance program counter past literal */
    _adjust_PC_by(4);
}

/* HALT ( -- ) Halt the virtual CPU. */
static void op_HALT(mk_context_t * ctx) {
    ctx->halted = 1;
}

/* TRON ( -- ) Enable debug tracing. */
static void op_TRON(mk_context_t * ctx) {
    ctx->DbgTraceEnable = 1;
}

/* TROFF ( -- ) Disable debug tracing. */
static void op_TROFF(mk_context_t * ctx) {
    ctx->DbgTraceEnable = 0;
}

/* DUMP ( -- ) Hexdump S bytes of RAM starting at address T, drop S & T. */
static void op_DUMP(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2);
    u32 firstAddr = ctx->T;
    u32 lastAddr = ctx->T + ctx->S - 1;
    _assert_valid_address(lastAddr);
    _drop_S_and_T();
    u8 col = 0;
    u32 addr;
    mk_str_t left = {0, {0}};
    mk_str_t right = {0, {0}};
    /* Example: 0000  41414141 424242 43434343 44444444  AAAA BBBB CCCC DDDD */
    for(addr = firstAddr; addr <= lastAddr; addr++) {
        u8 data = _peek_u8(addr);
        switch(col) {
            case 0: /* Insert address before first data byte of the row */
                fmt_hex_u16(&left, addr);
                fmt_spaces(&left, 2);
                break;
            case 4:
            case 8:
            case 12: /* Add space before 4th, 8th, and 12th bytes of row */
                fmt_spaces(&left, 1);
                fmt_spaces(&right, 1);
                break;
        }
        /* Append hex format to the left-side buffer */
        fmt_hex_u8(&left, data);
        /* Append ASCII format to right-side buffer       */
        /* Non-printable characters get replaced with '.' */
        if((data >= 32) && (data < 127)) {
            fmt_raw_byte(&right, data);
        } else {
            fmt_raw_byte(&right, (u8) '.');
        }
        /* After 16th byte of each row, print then clear both buffers */
        if(col == 15) {
            fmt_spaces(&left, 2);
            fmt_concat(&left, &right);
            fmt_newline(&left);
            vm_stdout_write(&left);
            left.len = 0;
            right.len = 0;
        }
        /* Advance the column counter (16 data columns per row) */
        col = (col + 1) & 15;
    }
    /* If dump size was not an even multiple of 16, print the last partial  */
    /* line with padding of spaces between left-side and right-side buffers */
    if(left.len > 0) {
        int pad = 41 - left.len + 2;
        pad = pad < 0 ? 0 : pad;
        fmt_spaces(&left, (u8) pad);
        fmt_concat(&left, &right);
        fmt_newline(&left);
        vm_stdout_write(&left);
    }
}

/* KEY ( -- [u8] bool ) Read the next byte from stdin.
 *    Stack effects depend on whether an input byte was available or not:
 *    1. Got a byte: ( -- u8 -1 ) S=u8, T=true means read was successful
 *    2. End of file:    ( -- 0 ) T=false means end of file
 */
static void op_KEY(mk_context_t * ctx) {
    u8 data;
    u8 eof = mk_host_getchar(&data);
    if(eof) {
        op_FALSE(ctx);
    } else {
        _assert_data_stack_is_not_full();
        _push_T(data);
        op_TRUE(ctx);
    }
}

/* IORH ( -- ) Hexdump the return stack */
static void op_IORH(mk_context_t * ctx) {
    /* TODO: Implement this */
}

/* MTR ( T -- ) Move T to R. */
static void op_MTR(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    _assert_return_stack_is_not_full();
    _push_R(ctx->T);
    _drop_T();
}

/* R ( -- r ) Push a copy of the top of the return stack (R) as T. */
static void op_R(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    _push_T(ctx->R);
}

/* CALL ( -- ) Call subroutine at address T, pushing old PC to return stack. */
static void op_CALL(mk_context_t * ctx) {
    _assert_return_stack_is_not_full();
    _assert_data_stack_depth_is_at_least(1);
    _push_R(ctx->PC);
    ctx->PC = ctx->T;
    _drop_T();
}

/* PC ( -- pc ) Push value of program counter (PC) register as T. */
static void op_PC(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    _push_T(ctx->PC);
}

/* MTE ( err -- ) Move value from T into the ERR register (raise an error). */
static void op_MTE(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    ctx->err = ctx->T;
    _drop_T();
}

/* LB ( addr -- i8 ) Load i8 (signed byte) at address T into T as an i32. */
static void op_LB(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    _assert_valid_address(ctx->T);
    u16 address = ctx->T;
    i32 data = (i32) _peek_u8(address);
    ctx->T = data;
}

/* SB ( u8 addr -- ) Store low byte of S (u8) into address T, drop S & T. */
static void op_SB(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2);
    _assert_valid_address(ctx->T);
    u16 address = ctx->T;
    u8 data = (u8) ctx->S;
    _poke_u8(data, address);
    _drop_S_and_T();
}

/* LH ( addr -- i32 ) Load i16 (signed halfword) at address T into T as an i32.
 */
static void op_LH(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    u16 address = ctx->T;
    _assert_valid_address(address + 1);
    i32 data = (i32) ((i16) _peek_u16(address));
    ctx->T = data;
}

/* SH ( i16 addr -- ) Store u16 (unsigned halfword) from S into address T. */
static void op_SH(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2);
    _assert_valid_address(ctx->T);
    u16 address = ctx->T;
    u32 data = (u16) ctx->S;
    _poke_u16(data, address);
    _drop_S_and_T();
}

/* LW ( addr -- i32 ) Load i32 (signed word) at address T into T. */
static void op_LW(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    u16 address = ctx->T;
    _assert_valid_address(address + 3);
    i32 data = (i32) _peek_u32(address);
    ctx->T = data;
}

/* SW ( u32 addr -- ) Store u32 (unsigned word) from S into address T. */
static void op_SW(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2);
    _assert_valid_address(ctx->T);
    u16 address = ctx->T;
    u32 data = (u32) ctx->S;
    _poke_u32(data, address);
    _drop_S_and_T();
}

/* ADD ( S T -- S+T ) Store S+T in T, nip S. */
static void op_ADD(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S + ctx->T);
}

/* SUB ( S T -- S-T ) Store S-T in T, nip S. */
static void op_SUB(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S - ctx->T);
}

/* MUL ( S T -- S*T ) Store S*T in T, nip S. */
static void op_MUL(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S * ctx->T);
}

/* DIV ( S T -- S/T ) Store S/T in T, nip S. (CAUTION! integer division) */
static void op_DIV(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S / ctx->T);
}

/* MOD ( S T -- S%T ) Store S modulo T in T, nip S. */
static void op_MOD(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S % ctx->T);
}

/* SLL ( S T -- S<<T ) Store S logical left-shifted by T bits in T, nip S. */
static void op_SLL(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S << ctx->T);
}

/* SRL ( S T -- S>>T ) Store S logical right-shifted by T bits in T, nip S.
 *     This is the shift to use if you want zero-fill on the left.
 */
static void op_SRL(mk_context_t * ctx) {
    _apply_lambda_ST(((u32)ctx->S) >> ctx->T);
}

/* SRA ( S T -- S>>>T ) Store S arithmetic-shifted by T bits in T, nip S.
 *     This is the shift to use if you want sign-bit-fill on the left.
 *     CAUTION! This seems to be a murky area of the C spec. Possible UB.
 */
static void op_SRA(mk_context_t * ctx) {
    _apply_lambda_ST(((i32)ctx->S) >> ctx->T);
}

/* INV ( n -- ~n ) Do a bitwise inversion of each bit in T. */
static void op_INV(mk_context_t * ctx) {
    _apply_lambda_T(~ (ctx->T));
}

/* XOR ( S T -- S^T ) Calculate S bitwise_XOR T. */
static void op_XOR(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S ^ ctx->T);
}

/* OR ( S T -- S|T ) Calculate S bitwise_OR T, which, in Markab, is also a
 *     logical OR because we're using Forth-style truth values.
 */
static void op_OR(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S | ctx->T);
}

/* AND ( S T -- S&T ) Calculate S bitwise_AND T, which, in Markab, is also a
 *     logical AND because we're using Forth-style truth values.
 */
static void op_AND(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S & ctx->T);
}

/* GT ( S T -- S>T ) Test S>T with Forth-style truth value (true is -1). */
static void op_GT(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S > ctx->T ? -1 : 0);
}

/* LT ( S T -- S<T ) Test S<T with Forth-style truth value (true is -1). */
static void op_LT(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S < ctx->T ? -1 : 0);
}

/* EQ ( S T -- S==T ) Test S==T with Forth-style truth value (true is -1). */
static void op_EQ(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S != ctx->T ? -1 : 0);
}

/* NE ( S T -- S!=T ) Test S!=T with Forth-style truth value (true is -1). */
static void op_NE(mk_context_t * ctx) {
    _apply_lambda_ST(ctx->S != ctx->T ? -1 : 0);
}

/* ZE ( n -- n==0 ) Test T==0 with Forth-style truth value (true is -1). */
static void op_ZE(mk_context_t * ctx) {
    _apply_lambda_T(ctx->T == 0 ? -1 : 0);
}

/* INC ( n -- n+1 ) Increment the value in T. */
static void op_INC(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1);
    ctx->T += 1;
}

/* DEC ( n -- n-1 ) Decrement the value in T. */
static void op_DEC(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1);
    ctx->T -= 1;
}

/* EMIT ( u8 -- ) Write the low byte of T to stdout. */
static void op_EMIT(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    mk_host_putchar((u8)ctx->T);
    _drop_T();
}

/* DOT ( i32 -- ) */
static void op_DOT(mk_context_t * ctx) {
    /* TODO: Implement this */
}

/* IODH ( -- ) */
static void op_IODH(mk_context_t * ctx) {
    /* TODO: Implement this */
}

/* IOD ( -- ) */
static void op_IOD(mk_context_t * ctx) {
    /* TODO: Implement this */
}

/* RDROP ( -- ) Drop R, the top item of the return stack. */
static void op_RDROP(mk_context_t * ctx) {
    _assert_return_stack_depth_is_at_least(1);
    _drop_R();
}

/* DROP ( n -- ) Drop T, the top item of the data stack. */
static void op_DROP(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    _drop_T();
}

/* DUP ( n1 -- n1 n1 ) Duplicate Top item of data stack (push a copy of T). */
static void op_DUP(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    _assert_data_stack_is_not_full();
    _push_T(ctx->T);
}

/* OVER ( n1 n2 -- n1 n2 n1 ) Push a copy of Second data stack item. */
static void op_OVER(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2);
    _assert_data_stack_is_not_full();
    _push_T(ctx->S);
}

/* SWAP ( n1 n2 -- n2 n1 ) Swap the Second and Top items on the data stack. */
static void op_SWAP(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(2);
    i32 n = ctx->T;
    ctx->T = ctx->S;
    ctx->S = n;
}

/* MTA ( T -- ) Move the value of T into the A register. */
static void op_MTA(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    ctx->A = ctx->T;
    _drop_T();
}

/* LBA ( -- n ) Load byte from address in register A. */
static void op_LBA(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    _assert_valid_address(ctx->A);
    u16 address = ctx->A;
    u32 data = (u32) _peek_u8(address);
    _push_T(data);
}

/* LBAI ( -- n ) Load byte from address in register A, increment A. */
static void op_LBAI(mk_context_t * ctx) {
    op_LBA(ctx);
    ctx->A += 1;
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
    _assert_data_stack_is_not_full();
    _push_T(ctx->A);
}

/* MTB ( T -- ) Move the value of T into the B register. */
static void op_MTB(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    ctx->B = ctx->T;
    _drop_T();
}

/* LBB ( -- n ) Load byte from address in register B */
static void op_LBB(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    _assert_valid_address(ctx->B);
    u16 address = ctx->B;
    u32 data = (u32) _peek_u8(address);
    _push_T(data);
}

/* LBBI ( -- n ) Load byte from address in register B, increment B. */
static void op_LBBI(mk_context_t * ctx) {
    op_LBB(ctx);
    ctx->B += 1;
}

/* SBBI ( n -- ) Store low byte of T to address in register B, increment B. */
static void op_SBBI(mk_context_t * ctx) {
    _assert_data_stack_depth_is_at_least(1);
    _assert_valid_address(ctx->B);
    u16 address = ctx->B;
    u8 data = (u8) ctx->T;
    _poke_u8(data, address);
    _drop_T();
    ctx->B += 1;
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
    _assert_data_stack_is_not_full();
    _push_T(ctx->B);
}

/* TRUE ( -- -1 ) Push the Forth-style truth value, -1 (all bits set). */
static void op_TRUE(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    _push_T(-1);
}

/* FALSE ( -- 0 ) Push the Forth-style false value, 0 (all bits clear). */
static void op_FALSE(mk_context_t * ctx) {
    _assert_data_stack_is_not_full();
    _push_T(0);
}


#endif /* LIBMKB_OP_C */
