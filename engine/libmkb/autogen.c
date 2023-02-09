/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * THIS FILE IS AUTOMATICALLY GENERATED
 * DO NOT MAKE EDITS HERE
 * See codegen.py for details
 */
#ifndef LIBMKB_AUTOGEN_C
#define LIBMKB_AUTOGEN_C

#include "libmkb.h"
#include "autogen.h"

/* Markab VM opcode dictionary */
static const char * const opcodes[MK_OPCODES_LEN] = {
    "NOP",     /*  0 */
    "HALT",    /*  1 */
    "U8",      /*  2 */
    "U16",     /*  3 */
    "I32",     /*  4 */
    "STR",     /*  5 */
    "BZ",      /*  6 */
    "JMP",     /*  7 */
    "JAL",     /*  8 */
    "RET",     /*  9 */
    "CALL",    /* 10 */
    "LB",      /* 11 */
    "SB",      /* 12 */
    "LH",      /* 13 */
    "SH",      /* 14 */
    "LW",      /* 15 */
    "SW",      /* 16 */
    "INC",     /* 17 */
    "DEC",     /* 18 */
    "ADD",     /* 19 */
    "SUB",     /* 20 */
    "MUL",     /* 21 */
    "DIV",     /* 22 */
    "MOD",     /* 23 */
    "SLL",     /* 24 */
    "SRL",     /* 25 */
    "SRA",     /* 26 */
    "INV",     /* 27 */
    "XOR",     /* 28 */
    "OR",      /* 29 */
    "AND",     /* 30 */
    "GT",      /* 31 */
    "LT",      /* 32 */
    "EQ",      /* 33 */
    "NE",      /* 34 */
    "ZE",      /* 35 */
    "TRUE",    /* 36 */
    "FALSE",   /* 37 */
    "DROP",    /* 38 */
    "DUP",     /* 39 */
    "OVER",    /* 40 */
    "SWAP",    /* 41 */
    "R",       /* 42 */
    "MTR",     /* 43 */
    "RDROP",   /* 44 */
    "EMIT",    /* 45 */
    "PRINT",   /* 46 */
    "CR",      /* 47 */
    "DOT",     /* 48 */
    "DOTH",    /* 49 */
    "DOTS",    /* 50 */
    "DOTSH",   /* 51 */
    "DOTRH",   /* 52 */
    "DUMP",    /* 53 */
};

/* Markab language core vocabulary */
static const mk_voc_item_t core_voc[MK_CORE_VOC_LEN] = {
    { {"Heap"},        MK_T_CONST, 0x0000    },
    { {"HeapRes"},     MK_T_CONST, 0xE000    },
    { {"HeapMax"},     MK_T_CONST, 0xE0FF    },
    { {"DP"},          MK_T_CONST, 0xE100    },
    { {"MemMax"},      MK_T_CONST, 0xFFFF    },
    { {"T_VAR"},       MK_T_CONST, 0         },
    { {"T_CONST"},     MK_T_CONST, 1         },
    { {"T_OP"},        MK_T_CONST, 2         },
    { {"T_OBJ"},       MK_T_CONST, 3         },
    { {"T_IMM"},       MK_T_CONST, 4         },
    { {"nop"},         MK_T_OP,    MK_NOP    },
    { {"halt"},        MK_T_OP,    MK_HALT   },
    { {"call"},        MK_T_OP,    MK_CALL   },
    { {"@"},           MK_T_OP,    MK_LB     },
    { {"!"},           MK_T_OP,    MK_SB     },
    { {"h@"},          MK_T_OP,    MK_LH     },
    { {"h!"},          MK_T_OP,    MK_SH     },
    { {"w@"},          MK_T_OP,    MK_LW     },
    { {"w!"},          MK_T_OP,    MK_SW     },
    { {"1+"},          MK_T_OP,    MK_INC    },
    { {"1-"},          MK_T_OP,    MK_DEC    },
    { {"+"},           MK_T_OP,    MK_ADD    },
    { {"-"},           MK_T_OP,    MK_SUB    },
    { {"*"},           MK_T_OP,    MK_MUL    },
    { {"/"},           MK_T_OP,    MK_DIV    },
    { {"%"},           MK_T_OP,    MK_MOD    },
    { {"<<"},          MK_T_OP,    MK_SLL    },
    { {">>"},          MK_T_OP,    MK_SRL    },
    { {">>>"},         MK_T_OP,    MK_SRA    },
    { {"~"},           MK_T_OP,    MK_INV    },
    { {"^"},           MK_T_OP,    MK_XOR    },
    { {"|"},           MK_T_OP,    MK_OR     },
    { {"&"},           MK_T_OP,    MK_AND    },
    { {">"},           MK_T_OP,    MK_GT     },
    { {"<"},           MK_T_OP,    MK_LT     },
    { {"="},           MK_T_OP,    MK_EQ     },
    { {"!="},          MK_T_OP,    MK_NE     },
    { {"0="},          MK_T_OP,    MK_ZE     },
    { {"true"},        MK_T_OP,    MK_TRUE   },
    { {"false"},       MK_T_OP,    MK_FALSE  },
    { {"drop"},        MK_T_OP,    MK_DROP   },
    { {"dup"},         MK_T_OP,    MK_DUP    },
    { {"over"},        MK_T_OP,    MK_OVER   },
    { {"swap"},        MK_T_OP,    MK_SWAP   },
    { {"r"},           MK_T_OP,    MK_R      },
    { {">r"},          MK_T_OP,    MK_MTR    },
    { {"rdrop"},       MK_T_OP,    MK_RDROP  },
    { {"emit"},        MK_T_OP,    MK_EMIT   },
    { {"print"},       MK_T_OP,    MK_PRINT  },
    { {"cr"},          MK_T_OP,    MK_CR     },
    { {"."},           MK_T_OP,    MK_DOT    },
    { {".h"},          MK_T_OP,    MK_DOTH   },
    { {".S"},          MK_T_OP,    MK_DOTS   },
    { {".Sh"},         MK_T_OP,    MK_DOTSH  },
    { {".Rh"},         MK_T_OP,    MK_DOTRH  },
    { {"dump"},        MK_T_OP,    MK_DUMP   },
};

/*
 * This is the bytecode interpreter. The for-loop here is a very, very hot code
 * path, so we need to be careful to help the compiler optimize it well. With
 * that in mind, this code expects to be #included into libmkb.c, which also
 * #includes op.c. That arrangement allows the compiler to inline opcode
 * implementations into the big switch statement.
 */
static void autogen_step(mk_context_t * ctx) {
    int i; /* declare outside of for loop for ANSI C compatibility */
    for(i=0; i<MK_MAX_CYCLES; i++) {
        switch(vm_next_instruction(ctx)) {
            case 0:
                op_NOP();
                break;
            case 1:
                op_HALT(ctx);
                break;
            case 2:
                op_U8(ctx);
                break;
            case 3:
                op_U16(ctx);
                break;
            case 4:
                op_I32(ctx);
                break;
            case 5:
                op_STR(ctx);
                break;
            case 6:
                op_BZ(ctx);
                break;
            case 7:
                op_JMP(ctx);
                break;
            case 8:
                op_JAL(ctx);
                break;
            case 9:
                op_RET(ctx);
                break;
            case 10:
                op_CALL(ctx);
                break;
            case 11:
                op_LB(ctx);
                break;
            case 12:
                op_SB(ctx);
                break;
            case 13:
                op_LH(ctx);
                break;
            case 14:
                op_SH(ctx);
                break;
            case 15:
                op_LW(ctx);
                break;
            case 16:
                op_SW(ctx);
                break;
            case 17:
                op_INC(ctx);
                break;
            case 18:
                op_DEC(ctx);
                break;
            case 19:
                op_ADD(ctx);
                break;
            case 20:
                op_SUB(ctx);
                break;
            case 21:
                op_MUL(ctx);
                break;
            case 22:
                op_DIV(ctx);
                break;
            case 23:
                op_MOD(ctx);
                break;
            case 24:
                op_SLL(ctx);
                break;
            case 25:
                op_SRL(ctx);
                break;
            case 26:
                op_SRA(ctx);
                break;
            case 27:
                op_INV(ctx);
                break;
            case 28:
                op_XOR(ctx);
                break;
            case 29:
                op_OR(ctx);
                break;
            case 30:
                op_AND(ctx);
                break;
            case 31:
                op_GT(ctx);
                break;
            case 32:
                op_LT(ctx);
                break;
            case 33:
                op_EQ(ctx);
                break;
            case 34:
                op_NE(ctx);
                break;
            case 35:
                op_ZE(ctx);
                break;
            case 36:
                op_TRUE(ctx);
                break;
            case 37:
                op_FALSE(ctx);
                break;
            case 38:
                op_DROP(ctx);
                break;
            case 39:
                op_DUP(ctx);
                break;
            case 40:
                op_OVER(ctx);
                break;
            case 41:
                op_SWAP(ctx);
                break;
            case 42:
                op_R(ctx);
                break;
            case 43:
                op_MTR(ctx);
                break;
            case 44:
                op_RDROP(ctx);
                break;
            case 45:
                op_EMIT(ctx);
                break;
            case 46:
                op_PRINT(ctx);
                break;
            case 47:
                op_CR();
                break;
            case 48:
                op_DOT(ctx);
                break;
            case 49:
                op_DOTH(ctx);
                break;
            case 50:
                op_DOTS(ctx);
                break;
            case 51:
                op_DOTSH(ctx);
                break;
            case 52:
                op_DOTRH(ctx);
                break;
            case 53:
                op_DUMP(ctx);
                break;
            default:
                vm_irq_err(ctx, MK_ERR_BAD_OPCODE);
                ctx->halted = 1;
        };
        if(ctx->halted) {
            return;
        }
    }
    /* Making it this far means the MK_MAX_CYCLES limit was exceeded */
    vm_irq_err(ctx, MK_ERR_CPU_HOG);
    autogen_step(ctx);
};

#endif /* LIBMKB_AUTOGEN_C */
