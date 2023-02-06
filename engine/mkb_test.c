/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Test libmkb's VM and compiler.
 */
#ifdef PLAN_9
/* Plan 9 inlcudes */
#  include <u.h>              /* u8int, u16int, u32int, ... */
#  include <libc.h>           /* print(), exits(), memset(), ... */
#  include <stdio.h>          /* getchar(), putchar() */
#else
/* POSIX includes */
#  ifndef __MACH__
/*   This unlocks snprintf() powers on Debian since I'm using `clang -ansi`. */
/*   Conversely, on macOS, this actually makes clang mad. I hesitate to just */
/*   crank up the _XOPEN_SOURCE level even higher because I want to be       */
/*   warned if I try to use C99 features that might cause trouble on Plan 9. */
#    define _XOPEN_SOURCE 500
#  endif
#  include <stdint.h>         /* uint8_t, uint16_t, int32_t, ... */
#  include <stdio.h>          /* printf(), putchar(), ... */
#  include <string.h>         /* memset() */
#  include <unistd.h>         /* STDOUT_FILENO */
#endif
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"


/* ============================== */
/* == Test scoring global vars == */
/* ============================== */

/* Typedef for a jumbo-size counted string buffer */
#define MKB_TEST_StrBufSize (1024 * 16)
typedef struct mkb_test_str {
    u16 len;
    u8 buf[MKB_TEST_StrBufSize];
} mkb_test_str_t;

/* Global buffer used for capturing the VM's writes to stdout during tests */
static mkb_test_str_t TEST_STDOUT;

/* Global buffer to hold names of failing tests */
mkb_test_str_t FAIL_LOG = {0, {0}};

/* Global var for counting passed tests */
static int TEST_SCORE_PASS = 0;

/* Global var for counting failed tests */
static int TEST_SCORE_FAIL = 0;


/* ==================================== */
/* == Test scoring utility functions == */
/* ==================================== */

/* Clear the buffer which captures VM writes to stdout during each test. */
static void test_stdout_reset(void) {
    memset((void *)&TEST_STDOUT, 0, sizeof(mkb_test_str_t));
}

/* Check for match between expected string and TEST_STDOUT.
 * Returns: 1 when strings match, 0 when strings do not match
 */
static int test_stdout_match(const char *expected) {
    long e_len = strlen(expected);
    /* Check if the expected and actual lengths match */
    if(e_len != TEST_STDOUT.len) {
        return 0;  /* FAIL */
    }
    /* Check if expected and actual characters match */
    int i;
    for(i = 0; i < TEST_STDOUT.len; i++) {
        if(expected[i] != TEST_STDOUT.buf[i]) {
            return 0;  /* FAIL */
        }
    }
    return 1;  /* Match! */
}

/* Record score for passed test and print the pass message */
static void score_pass(const char * name) {
    TEST_SCORE_PASS += 1;
    const char * fmt = "[%s: pass]\n\n";
#ifdef PLAN_9
    print(fmt, name);
#else
    printf(fmt, name);
#endif
}

/* Record score for failed test and print the FAIL message */
static void score_fail(const char * name) {
    TEST_SCORE_FAIL += 1;
    const char * fmt = "[%s: FAIL]\n\n";
    /* First log the message to stdout */
#ifdef PLAN_9
    print(fmt, name);
#else
    printf(fmt, name);
#endif
    /* Then append the message to FAIL_LOG */
    char buf[128];
    snprintf(buf, sizeof(buf), "[  %-20s ]\n", name);
    int length = strlen(buf);
    if(FAIL_LOG.len + length < sizeof(FAIL_LOG.buf)) {
        memcpy((void *)&(FAIL_LOG.buf[FAIL_LOG.len]), buf, length);
        FAIL_LOG.len += length;
    }
}


/* ============================================== */
/* == Libmkb Host API implementation functions == */
/* ============================================== */

/* Log an error code to stdout and TEST_STDOUT */
void mk_host_log_error(u8 error_code) {
    /* Translate the error code into a more useful description */
    char tag[64];
    switch(error_code) {
        case MK_ERR_OK:
            snprintf(tag, sizeof(tag), "OK, no error");
            break;
        case MK_ERR_D_OVER:
            snprintf(tag, sizeof(tag), "Stack overflow");
            break;
        case MK_ERR_D_UNDER:
            snprintf(tag, sizeof(tag), "Stack underflow");
            break;
        case MK_ERR_R_OVER:
            snprintf(tag, sizeof(tag), "Return stack overflow");
            break;
        case MK_ERR_R_UNDER:
            snprintf(tag, sizeof(tag), "Return stack underflow");
            break;
        case MK_ERR_BAD_ADDRESS:
            snprintf(tag, sizeof(tag), "Bad address");
            break;
        case MK_ERR_BAD_OPCODE:
            snprintf(tag, sizeof(tag), "Bad opcode");
            break;
        case MK_ERR_CPU_HOG:
            snprintf(tag, sizeof(tag), "Code was hogging CPU");
            break;
        default:
            snprintf(tag, sizeof(tag), "%d", error_code);
    }
    /* Log the error message to real stdout */
    const char * fmt = "ERROR: %s\n";
#ifdef PLAN_9
    print(fmt, tag);
#else
    printf(fmt, tag);
#endif
    /* Log the error message to TEST_STDOUT */
    char buf[128];
    snprintf(buf, sizeof(buf), fmt, tag);
    int length = strlen(buf);
    if(TEST_STDOUT.len + length < sizeof(TEST_STDOUT.buf)) {
        memcpy((void *)&(TEST_STDOUT.buf[TEST_STDOUT.len]), buf, length);
        TEST_STDOUT.len += length;
    }
}

/* Write length bytes from byte buffer buf to real stdout and TEST_STDOUT */
void mk_host_stdout_write(const void * buf, int length) {
    /* First write to real stdout */
    write(1 /* STDOUT */, buf, length);
    /* Then append a copy to the TEST_STDOUT */
    if(TEST_STDOUT.len + length < sizeof(TEST_STDOUT.buf)) {
        memcpy((void *)&(TEST_STDOUT.buf[TEST_STDOUT.len]), buf, length);
        TEST_STDOUT.len += length;
    }
}

/* Write byte to stdout and TEST_STDOUT */
void mk_host_putchar(u8 data) {
    /* First write to real stdout */
    putchar(data);
    /* Then append a copy to the TEST_STDOUT */
    if(TEST_STDOUT.len + 1 < sizeof(TEST_STDOUT.buf)) {
        TEST_STDOUT.buf[TEST_STDOUT.len] = data;
        TEST_STDOUT.len += 1;
    }
}


/* ================================================= */
/* == And, finally... the actual tests start here == */
/* ================================================= */

/* Macro: run code, check expected output, score results, reset TEST_STDOUT */
#define _score(NAME, CODE, EXPECT_S, EXPECT_E) {       \
    if(EXPECT_E != mk_load_rom(CODE, sizeof(CODE))) {  \
        score_fail(NAME);                              \
    } else {                                           \
        if(test_stdout_match(EXPECT_S)) {              \
            score_pass(NAME);                          \
        } else {                                       \
            score_fail(NAME);                          \
        }                                              \
    }                                                  \
    test_stdout_reset();                               }


/* =========== */
/* === NOP === */
/* =========== */

/* Test NOP opcode */
static void test_NOP(void) {
    u8 code[] = {
        MK_NOP, MK_PC, MK_DOTS,
        MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected = " 2\n";
    _score("test_NOP", code, expected, MK_ERR_OK);
}


/* ================== */
/* === VM Control === */
/* ================== */

/* Test RESET opcode */
static void test_RESET(void) {
    u8 code[] = {
        MK_U8, 255, MK_DUP, MK_DUP, MK_MTR,
        MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_U8, 255, MK_MTE,  /* Raise an error to be cleared by RESET */
        MK_RESET,
        MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected =
        " ff ff\n"
        " ff\n"
        "ERROR: 255\n"
        " Stack is empty\n"
        " Return stack is empty\n";
    _score("test_RESET", code, expected, MK_ERR_OK);
}

/* Test HALT opcode */
static void test_HALT(void) {
    u8 code[] = {
        MK_U8, 'A', MK_EMIT,
        MK_U8, '\n', MK_EMIT,
        MK_HALT,
        MK_U8, 'B', MK_EMIT,
        MK_U8, '\n', MK_EMIT,
    };
    char * expected = "A\n";
    _score("test_HALT", code, expected, MK_ERR_OK);
}

/* Test TRON opcode */
static void test_TRON(void) {
    u8 code[] = {
        MK_TRON, MK_NOP,
        MK_HALT,
    };
    /* TODO: Make a better test once tracing is implemented */
    char * expected = "";
    _score("test_TRON", code, expected, MK_ERR_OK);
}

/* Test TROFF opcode */
static void test_TROFF(void) {
    u8 code[] = {
        MK_TROFF, MK_NOP,
        MK_HALT,
    };
    /* TODO: Make a better test once tracing is implemented */
    char * expected = "";
    _score("test_TROFF", code, expected, MK_ERR_OK);
}

/* Test MTE opcode */
static void test_MTE(void) {
    u8 code[] = {
        MK_U8, 255, MK_DOT,
        MK_STR, 6, ' ', '>', 'e', 'r', 'r', '\n', MK_PRINT,
        MK_U8, 255, MK_MTE,
        MK_HALT,
    };
    char * expected =
        " 255 >err\n"
        "ERROR: 255\n";
    _score("test_MTE", code, expected, 255);
}


/* ================ */
/* === Literals === */
/* ================ */

/* Test U8 opcode */
static void test_U8(void) {
    u8 code[] = {
        MK_U8, 0, MK_U8, 1, MK_U8, 127, MK_U8, 128, MK_U8, 255,
        MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected = " 0 1 127 128 255\n";
    _score("test_U8", code, expected, MK_ERR_OK);
}

/* Test U16 opcode */
static void test_U16(void) {
    u8 code[] = {
        MK_U16,   0,   0,
        MK_U16,   1,   0,
        MK_U16, 127,   0,
        MK_U16, 128,   0,
        MK_U16, 255,   0,
        MK_U16,   0,   1,
        MK_U16,   0, 127,
        MK_U16,   0, 128,
        MK_U16, 255, 255,
        MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected = " 0 1 127 128 255 256 32512 32768 65535\n";
    _score("test_U16", code, expected, MK_ERR_OK);
}

/* Test I32 opcode */
static void test_I32(void) {
    u8 code[] = {
        MK_I32,   0,   0,   0,   0,
        MK_I32,   1,   0,   0,   0,
        MK_I32, 255, 255, 255, 127,
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 255,
        MK_DOTSH, MK_CR,
        MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected =
        " 0 1 7fffffff 80000000 ffffffff\n"
        " 0 1 2147483647 -2147483648 -1\n";
    _score("test_I32", code, expected, MK_ERR_OK);
}

/* Test STR opcode */
static void test_STR(void) {
    u8 code[] = {
        MK_STR, 13,
        'h', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '\n',
        MK_NOP,
        MK_PRINT,
        MK_HALT,
    };
    char * expected = "hello, world\n";
    _score("test_STR", code, expected, MK_ERR_OK);
}


/* ================================== */
/* === Branch, Jump, Call, Return === */
/* ================================== */

/* Test BZ opcode */
static void test_BZ(void) {
    u8 code[] = {
        MK_STR, 2, 'A', '\n', MK_PRINT,
        MK_U8, 0, MK_BZ, 6,                    /* skip the "B\n" */
        MK_STR, 2, 'B', '\n', MK_PRINT,
        MK_STR, 2, 'C', '\n', MK_PRINT,
        MK_U8, 1, MK_BZ, 6,                    /* don't skip the "D\n" */
        MK_STR, 2, 'D', '\n', MK_PRINT,
        MK_I32, 255, 255, 255, 255, MK_BZ, 6,  /* don't skip the "E\n" */
        MK_STR, 2, 'E', '\n', MK_PRINT,
        MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected =
        "A\n"
        "C\n"
        "D\n"
        "E\n"
        " Stack is empty\n";
    _score("test_BZ", code, expected, MK_ERR_OK);
}

/* Test JMP opcode */
static void test_JMP(void) {
    u8 code[] = {
        MK_JMP, 7, 0,                /* PC + 7 -> "B\n" */
        MK_U8, 'A', MK_EMIT, MK_CR,
        MK_HALT,
        MK_U8, 'B', MK_EMIT, MK_CR,
        MK_JMP, 246, 255,            /* PC + (-10) -> "A\n" */
        MK_HALT,
    };
    char * expected =
        "B\n"
        "A\n";
    _score("test_JMP", code, expected, MK_ERR_OK);
}

/* Test JAL opcode */
static void test_JAL(void) {
    u8 code[] = {
        MK_U8, 'A', MK_EMIT, MK_CR,
        /* call subroutine D at offset (+12) */
        MK_JAL, 12, 0,
        MK_U8, 'B', MK_EMIT, MK_CR,
        MK_HALT,
        /* subroutine C: */
            MK_U8, 'C', MK_EMIT, MK_CR,
            MK_RET,
        /* subroutine B: */
            MK_U8, 'D', MK_EMIT, MK_CR,
            /* call subroutine C at offset (-10) */
            MK_JAL, 246, 255,
            MK_RET,
        /* This should not be reachable */
        MK_STR, 5, 'o', 'h', ' ', 'n', 'o', '!', '\n', MK_PRINT,
        MK_HALT,
    };
    char * expected =
        "A\n"
        "D\n"
        "C\n"
        "B\n";
    _score("test_JAL", code, expected, MK_ERR_OK);
}

/* Test RET opcode */
static void test_RET(void) {
    u8 code[] = {
        MK_STR, 2, 'A', '\n', MK_PRINT,
        MK_U16, 16, 0, MK_MTR, MK_RET,   /* Return to the future! */
        MK_STR, 2, 'B', '\n', MK_PRINT,
        MK_HALT,
        MK_STR, 10, 's', 'u', 'r', 'p', 'r', 'i', 's', 'e', '!', '\n',
        MK_PRINT,
        MK_U16, 10, 0, MK_MTR, MK_RET,  /* Return to the past */
        MK_HALT,
    };
    char * expected =
        "A\n"
        "surprise!\n"
        "B\n";
    _score("test_RET", code, expected, MK_ERR_OK);
}

/* Test CALL opcode */
static void test_CALL(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_CALL", code, expected, MK_ERR_OK);
}


/* ======================================= */
/* === Memory Access: Loads and Stores === */
/* ======================================= */

/* Test LB opcode */
static void test_LB(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_LB", code, expected, MK_ERR_OK);
}

/* Test SB opcode */
static void test_SB(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_SB", code, expected, MK_ERR_OK);
}

/* Test LH opcode */
static void test_LH(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_LH", code, expected, MK_ERR_OK);
}

/* Test SH opcode */
static void test_SH(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_SH", code, expected, MK_ERR_OK);
}

/* Test LW opcode */
static void test_LW(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_LW", code, expected, MK_ERR_OK);
}

/* Test SW opcode */
static void test_SW(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_SW", code, expected, MK_ERR_OK);
}


/* ================== */
/* === Arithmetic === */
/* ================== */

/* Test INC opcode */
static void test_INC(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_INC", code, expected, MK_ERR_OK);
}

/* Test DEC opcode */
static void test_DEC(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_DEC", code, expected, MK_ERR_OK);
}

/* Test ADD opcode */
static void test_ADD(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_ADD", code, expected, MK_ERR_OK);
}

/* Test SUB opcode */
static void test_SUB(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_SUB", code, expected, MK_ERR_OK);
}

/* Test MUL opcode */
static void test_MUL(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_MUL", code, expected, MK_ERR_OK);
}

/* Test DIV opcode */
static void test_DIV(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_DIV", code, expected, MK_ERR_OK);
}

/* Test MOD opcode */
static void test_MOD(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_MOD", code, expected, MK_ERR_OK);
}


/* ============== */
/* === Shifts === */
/* ============== */

/* Test SLL opcode */
static void test_SLL(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_SLL", code, expected, MK_ERR_OK);
}

/* Test SRL opcode */
static void test_SRL(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_SRL", code, expected, MK_ERR_OK);
}

/* Test SRA opcode */
static void test_SRA(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_SRA", code, expected, MK_ERR_OK);
}


/* ======================== */
/* === Logic Operations === */
/* ======================== */

/* Test INV opcode */
static void test_INV(void) {
    u8 code[] = {
        MK_I32, 255, 255, 255, 255,
        MK_U8,    0,
        MK_I32,   0, 255, 255, 255,
        MK_DOTS, MK_CR,              /*  -1 0 -256 */
        MK_INV, MK_DOT,
        MK_INV, MK_DOT,
        MK_INV, MK_DOT,  MK_CR,      /*  255 -1 0 */
        MK_RESET, MK_INV,            /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " -1 0 -256\n"
        " 255 -1 0\n"
        "ERROR: Stack underflow\n";
    _score("test_INV", code, expected, MK_ERR_D_UNDER);
}

/* Test XOR opcode */
static void test_XOR(void) {
    u8 code[] = {
        MK_U8, 0x55, MK_TRUE, MK_DOTSH, MK_CR,  /*  55 ffffffff */
        MK_XOR,               MK_DOTSH, MK_CR,  /*  ffffffaa */
        MK_DUP, MK_XOR,       MK_DOTSH, MK_CR,  /*  0 */
        MK_XOR,                                 /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 55 ffffffff\n"
        " ffffffaa\n"
        " 0\n"
        "ERROR: Stack underflow\n";
    _score("test_XOR", code, expected, MK_ERR_D_UNDER);
}

/* Test OR opcode */
static void test_OR(void) {
    u8 code[] = {
        MK_U8, 0x55,
        MK_I32, 0xaa, 255, 255, 255,
        MK_DOTSH, MK_CR,              /*  55 ffffffaa */
        MK_OR,
        MK_DOTS, MK_CR,               /*  -1 */
        MK_OR,                        /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 55 ffffffaa\n"
        " -1\n"
        "ERROR: Stack underflow\n";
    _score("test_OR", code, expected, MK_ERR_D_UNDER);
}

/* Test AND opcode */
static void test_AND(void) {
    u8 code[] = {
        MK_TRUE, MK_TRUE, MK_DOTS, MK_CR,  /*  -1 -1 */
        MK_AND,           MK_DOTS, MK_CR,  /*  -1 */
        MK_FALSE,         MK_DOTS, MK_CR,  /*  -1 0 */
        MK_AND,           MK_DOTS, MK_CR,  /*  0 */
        MK_INV,
        MK_U16, 170, 170, MK_DOTSH, MK_CR,  /*  ffffffff aaaa */
        MK_AND,           MK_DOTSH, MK_CR,  /*  aaaa */
        MK_AND,                             /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " -1 -1\n"
        " -1\n"
        " -1 0\n"
        " 0\n"
        " ffffffff aaaa\n"
        " aaaa\n"
        "ERROR: Stack underflow\n";
    _score("test_AND", code, expected, MK_ERR_D_UNDER);
}


/* =================== */
/* === Comparisons === */
/* =================== */

/* Test GT opcode */
static void test_GT(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_GT", code, expected, MK_ERR_OK);
}

/* Test LT opcode */
static void test_LT(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_LT", code, expected, MK_ERR_OK);
}

/* Test EQ opcode */
static void test_EQ(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_EQ", code, expected, MK_ERR_OK);
}

/* Test NE opcode */
static void test_NE(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_NE", code, expected, MK_ERR_OK);
}

/* Test ZE opcode */
static void test_ZE(void) {
    u8 code[] = {
        MK_HALT,
    };
    char * expected = "TODO: IMPLEMENT THIS";
    _score("test_ZE", code, expected, MK_ERR_OK);
}


/* ============================= */
/* === Truth Value Constants === */
/* ============================= */

/* Test TRUE opcode */
static void test_TRUE(void) {
    u8 code[] = {
        MK_TRUE, MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected = " -1\n";
    _score("test_TRUE", code, expected, MK_ERR_OK);
}

/* Test FALSE opcode */
static void test_FALSE(void) {
    u8 code[] = {
        MK_FALSE, MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected = " 0\n";
    _score("test_FALSE", code, expected, MK_ERR_OK);
}


/* ============================= */
/* === Data Stack Operations === */
/* ============================= */

/* Test DROP opcode */
static void test_DROP(void) {
    u8 code[] = {
        MK_U8,  1, MK_DOTSH, MK_CR,
        MK_U8,  2, MK_DOTSH, MK_CR,
        MK_U8,  3, MK_DOTSH, MK_CR,
        MK_U8,  4, MK_DOTSH, MK_CR,
        MK_U8,  5, MK_DOTSH, MK_CR,
        MK_U8,  6, MK_DOTSH, MK_CR,
        MK_U8,  7, MK_DOTSH, MK_CR,
        MK_U8,  8, MK_DOTSH, MK_CR,
        MK_U8,  9, MK_DOTSH, MK_CR,
        MK_U8, 10, MK_DOTSH, MK_CR,
        MK_U8, 11, MK_DOTSH, MK_CR,
        MK_U8, 12, MK_DOTSH, MK_CR,
        MK_U8, 13, MK_DOTSH, MK_CR,
        MK_U8, 14, MK_DOTSH, MK_CR,
        MK_U8, 15, MK_DOTSH, MK_CR,
        MK_U8, 16, MK_DOTSH, MK_CR,
        MK_U8, 17, MK_DOTSH, MK_CR,
        MK_U8, 18, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,  /* This one will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1\n"
        " 1 2\n"
        " 1 2 3\n"
        " 1 2 3 4\n"
        " 1 2 3 4 5\n"
        " 1 2 3 4 5 6\n"
        " 1 2 3 4 5 6 7\n"
        " 1 2 3 4 5 6 7 8\n"
        " 1 2 3 4 5 6 7 8 9\n"
        " 1 2 3 4 5 6 7 8 9 a\n"
        " 1 2 3 4 5 6 7 8 9 a b\n"
        " 1 2 3 4 5 6 7 8 9 a b c\n"
        " 1 2 3 4 5 6 7 8 9 a b c d\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10 11\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10 11 12\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10 11\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e\n"
        " 1 2 3 4 5 6 7 8 9 a b c d\n"
        " 1 2 3 4 5 6 7 8 9 a b c\n"
        " 1 2 3 4 5 6 7 8 9 a b\n"
        " 1 2 3 4 5 6 7 8 9 a\n"
        " 1 2 3 4 5 6 7 8 9\n"
        " 1 2 3 4 5 6 7 8\n"
        " 1 2 3 4 5 6 7\n"
        " 1 2 3 4 5 6\n"
        " 1 2 3 4 5\n"
        " 1 2 3 4\n"
        " 1 2 3\n"
        " 1 2\n"
        " 1\n"
        " Stack is empty\n"
        "ERROR: Stack underflow\n"  ;
    _score("test_DROP", code, expected, MK_ERR_D_UNDER);
}

/* Test DUP opcode */
static void test_DUP(void) {
    u8 code[] = {
        MK_U8, 1,
        MK_DOTS, MK_CR,                  /*  1 */
        MK_DUP, MK_DUP, MK_DOTS, MK_CR,  /*  1 1 1 */
        MK_RESET, MK_DUP,                /* This one will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1\n"
        " 1 1 1\n"
        "ERROR: Stack underflow\n";
    _score("test_DUP", code, expected, MK_ERR_D_UNDER);
}

/* Test OVER opcode */
static void test_OVER(void) {
    u8 code[] = {
        MK_U8, 1, MK_U8, 2, MK_DOTS, MK_CR,  /*  1 2 */
        MK_OVER, MK_OVER,   MK_DOTS, MK_CR,  /*  1 2 1 2 */
        MK_RESET, MK_U8, 1, MK_OVER,         /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 2\n"
        " 1 2 1 2\n"
        "ERROR: Stack underflow\n";
    _score("test_OVER", code, expected, MK_ERR_D_UNDER);
}

/* Test SWAP opcode */
static void test_SWAP(void) {
    u8 code[] = {
        MK_U8, 1, MK_U8, 2, MK_DOTS, MK_CR,  /*  1 2 */
        MK_SWAP,            MK_DOTS, MK_CR,  /*  2 1 */
        MK_DROP, MK_SWAP,                    /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 2\n"
        " 2 1\n"
        "ERROR: Stack underflow\n";
    _score("test_SWAP", code, expected, MK_ERR_D_UNDER);
}

/* Test PC opcode */
static void test_PC(void) {
    u8 code[] = {
        MK_PC, MK_DOTS, MK_CR,  /*  1 */
        MK_PC, MK_DOTS, MK_CR,  /*  1 4 */
        MK_HALT,
    };
    char * expected =
        " 1\n"
        " 1 4\n";
    _score("test_PC", code, expected, MK_ERR_OK);
}


/* =============================== */
/* === Return Stack Operations === */
/* =============================== */

/* Test R opcode */
static void test_R(void) {
    u8 code[] = {
        MK_U8,  1, MK_MTR, MK_DOTRH, MK_CR,
        MK_R, MK_INC, MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected =
        " 1\n"
        " 2\n";
    _score("test_R", code, expected, MK_ERR_OK);
}

/* Test MTR opcode */
static void test_MTR(void) {
    u8 code[] = {
        MK_U8,  1, MK_MTR, MK_DOTRH, MK_CR,
        MK_HALT,
    };
    char * expected = " 1\n";
    _score("test_MTR", code, expected, MK_ERR_OK);
}

/* Test RDROP opcode */
static void test_RDROP(void) {
    u8 code[] = {
        MK_U8,  1, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  2, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  3, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  4, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  5, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  6, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  7, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  8, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8,  9, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 10, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 11, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 12, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 13, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 14, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 15, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 16, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 17, MK_MTR, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,  /* This one will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1\n"
        " 1 2\n"
        " 1 2 3\n"
        " 1 2 3 4\n"
        " 1 2 3 4 5\n"
        " 1 2 3 4 5 6\n"
        " 1 2 3 4 5 6 7\n"
        " 1 2 3 4 5 6 7 8\n"
        " 1 2 3 4 5 6 7 8 9\n"
        " 1 2 3 4 5 6 7 8 9 a\n"
        " 1 2 3 4 5 6 7 8 9 a b\n"
        " 1 2 3 4 5 6 7 8 9 a b c\n"
        " 1 2 3 4 5 6 7 8 9 a b c d\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10 11\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f 10\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e f\n"
        " 1 2 3 4 5 6 7 8 9 a b c d e\n"
        " 1 2 3 4 5 6 7 8 9 a b c d\n"
        " 1 2 3 4 5 6 7 8 9 a b c\n"
        " 1 2 3 4 5 6 7 8 9 a b\n"
        " 1 2 3 4 5 6 7 8 9 a\n"
        " 1 2 3 4 5 6 7 8 9\n"
        " 1 2 3 4 5 6 7 8\n"
        " 1 2 3 4 5 6 7\n"
        " 1 2 3 4 5 6\n"
        " 1 2 3 4 5\n"
        " 1 2 3 4\n"
        " 1 2 3\n"
        " 1 2\n"
        " 1\n"
        " Return stack is empty\n"
        "ERROR: Return stack underflow\n";
    _score("test_RDROP", code, expected, MK_ERR_R_UNDER);
}


/* ================== */
/* === Console IO === */
/* ================== */

/* Test EMIT opcode */
static void test_EMIT(void) {
    u8 code[] = {
        MK_U8, 'E', MK_EMIT,
        MK_U8, 'm', MK_EMIT,
        MK_U8, 'i', MK_EMIT,
        MK_U8, 't', MK_EMIT,
        MK_CR,
        MK_HALT,
    };
    char * expected = "Emit\n";
    _score("test_EMIT", code, expected, MK_ERR_OK);
}

/* Test HEX opcode */
static void test_HEX(void) {
    u8 code[] = {
        MK_U8, 255, MK_HEX, MK_DOT, MK_CR,
        MK_HALT,
    };
    char * expected = " ff\n";
    _score("test_HEX", code, expected, MK_ERR_OK);
}

/* Test DECIMAL opcode */
static void test_DECIMAL(void) {
    u8 code[] = {
        MK_U8, 255, MK_DECIMAL, MK_DOT, MK_CR,
        MK_HALT,
    };
    char * expected = " 255\n";
    _score("test_DECIMAL", code, expected, MK_ERR_OK);
}

/* Test BASE opcode */
static void test_BASE(void) {
    u8 code[] = {
        /* hex base decimal base .S */
        MK_HEX, MK_BASE, MK_DECIMAL, MK_BASE,
        MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected = " 16 10\n";
    _score("test_BASE", code, expected, MK_ERR_OK);
}

/* Test PRINT opcode */
static void test_PRINT(void) {
    u8 code[] = {
        MK_STR, 12,
        'p', 'r', 'i', 'n', 't', ' ', 'p', 'r', 'i', 'n', 't', '\n',
        MK_PRINT,
        MK_HALT,
    };
    char * expected = "print print\n";
    _score("test_PRINT", code, expected, MK_ERR_OK);
}

/* Test CR opcode */
static void test_CR(void) {
    u8 code[] = {
        MK_STR, 2, 'c', 'r',
        MK_PRINT,
        MK_CR,
        MK_HALT,
    };
    char * expected = "cr\n";
    _score("test_CR", code, expected, MK_ERR_OK);
}


/* ========================================= */
/* === Debug Dumps for Stacks and Memory === */
/* ========================================= */

/* Test DOT opcode */
static void test_DOT(void) {
    u8 code[] = {
        MK_DOTS, MK_CR,
        MK_U8,    0,
        MK_U8,    1,
        MK_U8,  255,
        MK_U16, 255, 255,
        MK_I32, 255, 255, 255, 127,
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 255,
        MK_DOTS, MK_CR,
        MK_DOT, MK_DOT, MK_DOT, MK_DOT, MK_DOT, MK_DOT, MK_DOT, MK_CR,
        MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected =
        " Stack is empty\n"
        " 0 1 255 65535 2147483647 -2147483648 -1\n"
        " -1 -2147483648 2147483647 65535 255 1 0\n"
        " Stack is empty\n";
    _score("test_DOT", code, expected, MK_ERR_OK);
}

/* Test DOTS opcode */
static void test_DOTS(void) {
    u8 code[] = {
        MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_DOTS, MK_CR,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_DOTS, MK_CR,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_DOTS, MK_CR,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_DOTS, MK_CR,
        MK_HALT,
    };
    char * expected =
        " Stack is empty\n"
        " 7951\n"
        " 7951 1\n"
        " 7951 1 28036591\n"
        " 7951 1 28036591 -508834321\n";
    _score("test_DOTS", code, expected, MK_ERR_OK);
}

/* Test DOTSH opcode */
static void test_DOTSH(void) {
    u8 code[] = {
        MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_DOTSH, MK_CR,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_DOTSH, MK_CR,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_DOTSH, MK_CR,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_DROP, MK_DOTSH, MK_CR,
        MK_HALT,
    };
    char * expected =
        " Stack is empty\n"
        " 1f0f\n"
        " 1f0f 1\n"
        " 1f0f 1 1abcdef\n"
        " 1f0f 1 1abcdef e1abcdef\n"
        " 1f0f 1 1abcdef\n"
        " 1f0f 1\n"
        " 1f0f\n"
        " Stack is empty\n";
    _score("test_DOTSH", code, expected, MK_ERR_OK);
}

/* Test DOTRH opcode */
static void test_DOTRH(void) {
    u8 code[] = {
        MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_MTR, MK_DOTRH, MK_CR,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_MTR, MK_DOTRH, MK_CR,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_MTR, MK_DOTRH, MK_CR,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_MTR, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_RDROP, MK_DOTRH, MK_CR,
        MK_HALT,
    };
    char * expected =
        " Return stack is empty\n"
        " 1f0f\n"
        " 1f0f 1\n"
        " 1f0f 1 1abcdef\n"
        " 1f0f 1 1abcdef e1abcdef\n"
        " 1f0f 1 1abcdef\n"
        " 1f0f 1\n"
        " 1f0f\n"
        " Return stack is empty\n";
    _score("test_DOTRH", code, expected, MK_ERR_OK);
}

/* Test DUMP opcode */
static void test_DUMP(void) {
    u8 n = 128;
    u8 code[] = {
        MK_U8, '0', MK_U8, n+ 0, MK_SB,
        MK_U8, '1', MK_U8, n+ 1, MK_SB,
        MK_U8, '2', MK_U8, n+ 2, MK_SB,
        MK_U8, '3', MK_U8, n+ 3, MK_SB,
        MK_U8, '4', MK_U8, n+ 4, MK_SB,
        MK_U8, '5', MK_U8, n+ 5, MK_SB,
        MK_U8, '6', MK_U8, n+ 6, MK_SB,
        MK_U8, '7', MK_U8, n+ 7, MK_SB,
        MK_U8, '8', MK_U8, n+ 8, MK_SB,
        MK_U8, '9', MK_U8, n+ 9, MK_SB,
        MK_U8, 'A', MK_U8, n+10, MK_SB,
        MK_U8, 'B', MK_U8, n+11, MK_SB,
        MK_U8, 'C', MK_U8, n+12, MK_SB,
        MK_U8, 'D', MK_U8, n+13, MK_SB,
        MK_U8, 'E', MK_U8, n+14, MK_SB,
        MK_U8, 'F', MK_U8, n+15, MK_SB,
        MK_U8,  32, MK_U8, n,    MK_DUMP,
        MK_U8,   1, MK_U8, n,    MK_DUMP,
        MK_U8,   5, MK_U8, n,    MK_DUMP,
        MK_U8,   9, MK_U8, n,    MK_DUMP,
        MK_U8,  13, MK_U8, n,    MK_DUMP,
        MK_U8,  15, MK_U8, n,    MK_DUMP,
        MK_HALT,
    };
    char * expected =
        "0080  30313233 34353637 38394142 43444546  0123 4567 89AB CDEF\n"
        "0090  00000000 00000000 00000000 00000000  .... .... .... ....\n"
        "0080  30                                   0\n"
        "0080  30313233 34                          0123 4\n"
        "0080  30313233 34353637 38                 0123 4567 8\n"
        "0080  30313233 34353637 38394142 43        0123 4567 89AB C\n"
        "0080  30313233 34353637 38394142 434445    0123 4567 89AB CDE\n";
    _score("test_DUMP", code, expected, MK_ERR_OK);
}


/* ========================================================================= */
/* === main() ============================================================== */
/* ========================================================================= */

#ifdef PLAN_9
void main() {
#else
int main() {
#endif
    /* Clear the buffer used to capture the VM's stdout writes during tests */
    test_stdout_reset();

    /* Run opcode tests */

    /* NOP */
    test_NOP();

    /* VM Control */
    test_RESET();
    test_HALT();
    test_TRON();
    test_TROFF();
    test_MTE();

    /* Integer Literals */
    test_U8();
    test_U16();
    test_I32();
    test_STR();

    /* Branch, Jump, Call, Return */
    test_BZ();
    test_JMP();
    test_JAL();
    test_RET();
    test_CALL();

    /* Memory Access: Loads and Stores */
    test_LB();
    test_SB();
    test_LH();
    test_SH();
    test_LW();
    test_SW();

    /* Arithmetic */
    test_INC();
    test_DEC();
    test_ADD();
    test_SUB();
    test_MUL();
    test_DIV();
    test_MOD();

    /* Shifts */
    test_SLL();
    test_SRL();
    test_SRA();

    /* Locic Operations */
    test_INV();
    test_XOR();
    test_OR();
    test_AND();

    /* Comparisons */
    test_GT();
    test_LT();
    test_EQ();
    test_NE();
    test_ZE();

    /* Truth Value Constants */
    test_TRUE();
    test_FALSE();

    /* Data Stack Operations */
    test_DROP();
    test_DUP();
    test_OVER();
    test_SWAP();
    test_PC();

    /* Return Stack Operations */
    test_R();
    test_MTR();
    test_RDROP();

    /* Console IO */
    test_EMIT();
    test_HEX();
    test_DECIMAL();
    test_BASE();
    test_PRINT();
    test_CR();

    /* Debug Dumps for Stacks and Memory */
    test_DOT();
    test_DOTS();
    test_DOTSH();
    test_DOTRH();
    test_DUMP();

    /* If any tests failed, print the failed test log */
    if(TEST_SCORE_FAIL > 0) {
        char * fail_header =
            "[=======================]\n"
            "[ List of Failing Tests ]\n";
        char * fail_footer = "[=======================]\n";
        write(1 /* STDOUT */, fail_header, strlen(fail_header));
        write(1 /* STDOUT */, FAIL_LOG.buf, FAIL_LOG.len);
        write(1 /* STDOUT */, fail_footer, strlen(fail_footer));
    }
    /* Summarize scores */
    char * fmt =
        "[==============]\n"
        "[ Test Results ]\n"
        "[ Pass: %3d    ]\n"
        "[ Fail: %3d    ]\n"
        "[==============]\n";
    char pass = (TEST_SCORE_FAIL == 0) ? 1 : 0;
#ifdef PLAN_9
    print(fmt, TEST_SCORE_PASS, TEST_SCORE_FAIL);
    if(pass) {
        exits(0);
    }
    exits("fail");
#else
    printf(fmt, TEST_SCORE_PASS, TEST_SCORE_FAIL);
    return pass ? 0 : 1;
#endif
}

