// Copyright (c) 2023 Sam Blenny
// SPDX-License-Identifier: MIT

/*
 * This file implements opcodes for the bytecode interpreter of the Markab VM's
 * stack machine CPU.
 */
#ifndef LIBMKB_OP_C
#define LIBMKB_OP_C

#include "autogen.h"
#include "op.h"

/*
 * ============================================================================
 * These macros reduce repetition of boilerplate code in opcode functions:
 * ============================================================================
 */

// Macro for asserting minimum data stack depth. If the assertion fails, this
// will raise a VM error interrupt and cause the enclosing function to return.
// Enclosing function must declare `mk_context_t * ctx`.
#define _assert_data_stack_depth_is_at_least(N) \
	if(ctx->DSDeep < N) {                       \
		vm_irq_err(ctx, MK_ERR_D_UNDER);        \
		return;                                 \
	}

// Macro for asserting maximum data stack depth. If the assertion fails, this
// will raise a VM error interrupt and cause the enclosing function to return.
// Enclosing function must declare `mk_context_t * ctx`.
#define _assert_data_stack_is_not_full() \
	if(ctx->DSDeep > 17) {               \
		vm_irq_err(ctx, MK_ERR_D_OVER);  \
		return;                          \
	}

// Macro for asserting minimum return stack depth. If the assertion fails, this
// will raise a VM error interrupt, reset the stacks, and cause the enclosing
// function to return. Enclosing function must declare `mk_context_t * ctx`.
#define _assert_return_stack_depth_is_at_least(N) \
	if(ctx->DSDeep < N) {                         \
		op_RESET(ctx);                            \
		vm_irq_err(ctx, MK_ERR_D_UNDER);          \
		return;                                   \
	}

// Macro for asserting maximum return stack depth. If the assertion fails, this
// will raise a VM error interrupt, reset the stacks, cause the enclosing
// function to return. Enclosing function must declare `mk_context_t * ctx`.
#define _assert_return_stack_is_not_full() \
	if(ctx->RSDeep > 16) {                 \
		op_RESET(ctx);                     \
		vm_irq_err(ctx, MK_ERR_R_OVER);    \
		return;                            \
	}

// Macro to nip (discard) S, second item of data stack, without checking stack
// depth. This is for macros or opcode functions that have already checked the
// stack depth. Enclosing function must declare `mk_context_t * ctx`.
#define _nip_S_without_minimum_stack_depth_check() \
	if(ctx->DSDeep > 2) {                          \
		u8 thirdOnStack = ctx->DSDeep - 3;         \
		ctx->S = ctx->DStack[thirdOnStack];        \
	}                                              \
	ctx->DSDeep -= 1;

// Macro to drop T, top item of the data stack, without checking stack depth.
// This is for macros or opcode functions that have already checked the stack
// depth. Enclosing function must declare `mk_context_t * ctx`.
#define _drop_T_without_minimum_stack_depth_check() \
	ctx->T = ctx->S;                                \
	_nip_S_without_minimum_stack_depth_check()

// Macro to drop R, top item of the return stack, without checking stack depth.
// This is for macros or opcode functions that have already checked the stack
// depth. Enclosing function must declare `mk_context_t * ctx`.
#define _rdrop_R_without_minimum_stack_depth_check()                 \
	if(ctx->RSDeep > 1) {                                            \
		ctx->R = ctx->RStack[ctx->RSDeep - 2 /* second on stack */]; \
	}                                                                \
	ctx->RSDeep -= 1;

// Macro to apply N=λ(S,T), storing the result in T and nipping S.
// Enclosing function must declare an instance of `mk_context_t * ctx`.
#define _apply_lambda_ST(N)                    \
	_assert_data_stack_depth_is_at_least(2)    \
	ctx->T = (N);                              \
	_nip_S_without_minimum_stack_depth_check()

// Macro to apply N=λ(T), storing the result in T.
// Enclosing function must declare an instance of `mk_context_t * ctx`.
#define _apply_lambda_T(N)                  \
	_assert_data_stack_depth_is_at_least(1) \
	ctx->T = (N);

// Macro to push N onto the data stack as a 32-bit signed integer, without
// checking maximum stack depth. This is for macros or opcode functions that
// have already checked the stack depth. Enclosing function must declare
// `mk_context_t * ctx`.
#define _push_T_without_max_stack_depth_check(N)                    \
	if(ctx->DSDeep > 1) {                                           \
		ctx->DStack[ctx->DSDeep - 2 /* third on stack */] = ctx->S; \
	}                                                               \
	ctx->S = ctx->T;                                                \
	ctx->T = N;                                                     \
	ctx->DSDeep += 1;

// Macro to push N onto the return stack as a 32-bit signed integer, without
// checking maximum stack depth. This is for macros or opcode functions that
// have already checked the stack depth. Enclosing function must declare
// `mk_context_t * ctx`.
#define _rpush_R_without_max_stack_depth_check(N)                    \
	if(ctx->RSDeep > 0) {                                            \
		ctx->RStack[ctx->RSDeep - 1 /* second on stack */] = ctx->R; \
	}                                                                \
	ctx->R = N;                                                      \
	ctx->RSDeep += 1;


/*
 * ============================================================================
 * These are opcode implementations:
 * ============================================================================
 */

// NOP ( -- ) Spend one virtual CPU clock cycle doing nothing.
static void op_NOP(mk_context_t * ctx) {
	/* Do nothing. On purpose. */
}

// RESET ( -- ) Reset data stack, return stack, error code, and input buffer.
static void op_RESET(mk_context_t * ctx) {
	ctx->DSDeep = 0;
	ctx->RSDeep = 0;
	ctx->err = 0;
	for(int i=0; i<MK_BufMax; i++) {
		ctx->InBuf[i] = 0;
	}
}

// JMP ( -- ) Jump to subroutine at address read from instruction stream.
//     The jump address is PC-relative to allow for relocatable object code.
static void op_JMP(mk_context_t * ctx) {
	u16 pc = ctx->PC;
	u16 n = (ctx->RAM[pc+1] << 8) | ctx->RAM[pc];  // signed LE halfword
	// Add offset to program counter to compute destination address.
    // CAUTION! CAUTION! CAUTION!
    // This is relying on u16 (uint16_t) integer overflow behavior to give
    // results modulo 0xffff to let us do math like (5-100) = 65441. This lets
    // signed 16-bit pc-relative offsets address the full memory range.
	ctx->PC += n;  // CAUTION! This is relying on an implicit modulo 0xffff
}

// JAL ( -- ) Jump to subroutine after pushing old value of PC to return stack.
//     The jump address is PC-relative to allow for relocatable object code.
static void op_JAL(mk_context_t * ctx) {
	_assert_return_stack_is_not_full()
	// Read a 16-bit signed offset (relative to PC) from instruction stream
	u16 pc = ctx->PC;
	u16 n = (ctx->RAM[pc+1] << 8) + ctx->RAM[pc];
    // Push the current Program Counter (PC) to return stack
	_rpush_R_without_max_stack_depth_check(pc + 2)
    // Add offset to program counter to compute destination address.
    // CAUTION! CAUTION! CAUTION!
    // This is relying on u16 (uint16_t) integer overflow behavior to give
    // results modulo 0xffff to let us do math like (5-100) = 65441. This lets
    // signed 16-bit pc-relative offsets address the full memory range.
	ctx->PC = pc + n;
}

// RET ( -- ) Return from subroutine, taking address from return stack.
static void op_RET(mk_context_t * ctx) {
	_assert_return_stack_depth_is_at_least(1)
	// Set program counter from top of return stack
	ctx->PC = (u8) ctx->R;
	// Drop R
	_rdrop_R_without_minimum_stack_depth_check()
}

// BZ ( T -- ) Branch to PC-relative address if T == 0, drop T.
//    The branch address is PC-relative to allow for relocatable object code.
static void op_BZ(mk_context_t * ctx) {
	_assert_data_stack_depth_is_at_least(1)
	u8 pc = ctx->PC;
	if(ctx->T == 0) {
		// Branch forward past conditional block: Add address literal from
		// instruction stream to PC. Maximum branch distance is +255.
		ctx->PC = pc + ctx->RAM[pc];
	} else {
		// Enter conditional block: Advance PC past address literal
		ctx->PC = pc + 1;
	}
	// Drop T the fast way
	_drop_T_without_minimum_stack_depth_check()
}

// BFOR ( -- )
static void op_BFOR(mk_context_t * ctx) {
}

// U8 ( -- )
static void op_U8(mk_context_t * ctx) {
}

// U16 ( -- )
static void op_U16(mk_context_t * ctx) {
}

// I32 ( -- )
static void op_I32(mk_context_t * ctx) {
}

// HALT ( -- ) Halt the virtual CPU
static void op_HALT(mk_context_t * ctx) {
	ctx->halted = 1;
}

// TRON ( -- )
static void op_TRON(mk_context_t * ctx) {
}

// TROFF ( -- )
static void op_TROFF(mk_context_t * ctx) {
}

// IODUMP ( -- )
static void op_IODUMP(mk_context_t * ctx) {
}

// IOKEY ( -- )
static void op_IOKEY(mk_context_t * ctx) {
}

// IORH ( -- )
static void op_IORH(mk_context_t * ctx) {
}

// IOLOAD ( -- )
static void op_IOLOAD(mk_context_t * ctx) {
}

// FOPEN ( -- )
static void op_FOPEN(mk_context_t * ctx) {
}

// FREAD ( -- )
static void op_FREAD(mk_context_t * ctx) {
}

// FWRITE ( -- )
static void op_FWRITE(mk_context_t * ctx) {
}

// FSEEK ( -- )
static void op_FSEEK(mk_context_t * ctx) {
}

// FTELL ( -- )
static void op_FTELL(mk_context_t * ctx) {
}

// FTRUNC ( -- )
static void op_FTRUNC(mk_context_t * ctx) {
}

// FCLOSE ( -- )
static void op_FCLOSE(mk_context_t * ctx) {
}

// MTR ( -- )
static void op_MTR(mk_context_t * ctx) {
}

// R ( -- )
static void op_R(mk_context_t * ctx) {
}

// CALL ( -- )
static void op_CALL(mk_context_t * ctx) {
}

// PC ( -- )
static void op_PC(mk_context_t * ctx) {
}

// MTE ( -- )
static void op_MTE(mk_context_t * ctx) {
}

// LB ( -- )
static void op_LB(mk_context_t * ctx) {
}

// SB ( -- )
static void op_SB(mk_context_t * ctx) {
}

// LH ( -- )
static void op_LH(mk_context_t * ctx) {
}

// SH ( -- )
static void op_SH(mk_context_t * ctx) {
}

// LW ( -- )
static void op_LW(mk_context_t * ctx) {
}

// SW ( -- )
static void op_SW(mk_context_t * ctx) {
}

// ADD ( -- )
static void op_ADD(mk_context_t * ctx) {
}

// SUB ( -- )
static void op_SUB(mk_context_t * ctx) {
}

// MUL ( -- )
static void op_MUL(mk_context_t * ctx) {
}

// DIV ( -- )
static void op_DIV(mk_context_t * ctx) {
}

// MOD ( -- )
static void op_MOD(mk_context_t * ctx) {
}

// SLL ( -- )
static void op_SLL(mk_context_t * ctx) {
}

// SRL ( -- )
static void op_SRL(mk_context_t * ctx) {
}

// SRA ( -- )
static void op_SRA(mk_context_t * ctx) {
}

// INV ( -- )
static void op_INV(mk_context_t * ctx) {
}

// XOR ( -- )
static void op_XOR(mk_context_t * ctx) {
}

// OR ( -- )
static void op_OR(mk_context_t * ctx) {
}

// AND ( -- )
static void op_AND(mk_context_t * ctx) {
}

// GT ( -- )
static void op_GT(mk_context_t * ctx) {
}

// LT ( -- )
static void op_LT(mk_context_t * ctx) {
}

// EQ ( -- )
static void op_EQ(mk_context_t * ctx) {
}

// NE ( -- )
static void op_NE(mk_context_t * ctx) {
}

// ZE ( -- )
static void op_ZE(mk_context_t * ctx) {
}

// INC ( -- )
static void op_INC(mk_context_t * ctx) {
}

// DEC ( -- )
static void op_DEC(mk_context_t * ctx) {
}

// IOEMIT ( -- )
static void op_IOEMIT(mk_context_t * ctx) {
}

// IODOT ( -- )
static void op_IODOT(mk_context_t * ctx) {
}

// IODH ( -- )
static void op_IODH(mk_context_t * ctx) {
}

// IOD ( -- )
static void op_IOD(mk_context_t * ctx) {
}

// RDROP ( -- )
static void op_RDROP(mk_context_t * ctx) {
}

// DROP ( -- ) Drop T, the top item of the data stack.
static void op_DROP(mk_context_t * ctx) {
	ctx->T = ctx->S;
	if(ctx->DSDeep > 2) {
		u8 third = ctx->DSDeep - 3;
		ctx->S = ctx->DStack[third];
	}
	ctx->DSDeep -= 1;
}

// DUP ( -- )
static void op_DUP(mk_context_t * ctx) {
}

// OVER ( -- )
static void op_OVER(mk_context_t * ctx) {
}

// SWAP ( -- )
static void op_SWAP(mk_context_t * ctx) {
}

// MTA ( -- )
static void op_MTA(mk_context_t * ctx) {
}

// LBA ( -- )
static void op_LBA(mk_context_t * ctx) {
}

// LBAI ( -- )
static void op_LBAI(mk_context_t * ctx) {
}

// AINC ( -- )
static void op_AINC(mk_context_t * ctx) {
}

// ADEC ( -- )
static void op_ADEC(mk_context_t * ctx) {
}

// A ( -- )
static void op_A(mk_context_t * ctx) {
}

// MTB ( -- )
static void op_MTB(mk_context_t * ctx) {
}

// LBB ( -- )
static void op_LBB(mk_context_t * ctx) {
}

// LBBI ( -- )
static void op_LBBI(mk_context_t * ctx) {
}

// SBBI ( -- )
static void op_SBBI(mk_context_t * ctx) {
}

// BINC ( -- )
static void op_BINC(mk_context_t * ctx) {
}

// BDEC ( -- )
static void op_BDEC(mk_context_t * ctx) {
}

// B ( -- )
static void op_B(mk_context_t * ctx) {
}

// TRUE ( -- )
static void op_TRUE(mk_context_t * ctx) {
}

// FALSE ( -- )
static void op_FALSE(mk_context_t * ctx) {
}


#endif /* LIBMKB_OP_C */
