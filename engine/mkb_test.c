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
    snprintf(buf, sizeof(buf), "[  %-22s ]\n", name);
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
        case MK_ERR_DIV_BY_ZERO:
            snprintf(tag, sizeof(tag), "Divide by zero");
            break;
        case MK_ERR_DIV_OVERFLOW:
            snprintf(tag, sizeof(tag), "Quotient would overflow");
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
    /* Round 1: ends with ERR_OK */
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

    /* Round 2: ends with ERR_BAD_ADDRESS */
    u8 code2[] = {
        MK_I32, 0, 0, 1, 0,
        MK_MTR, MK_DOTRH, MK_CR,        /*  10000 */
        MK_RET,                         /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        " 10000\n"
        "ERROR: Bad address\n";
    _score("test_RET_bad_addr", code2, expected2, MK_ERR_BAD_ADDRESS);
}

/* Test CALL opcode */
static void test_CALL(void) {
    /* This demonstrates a nested chain of function calls */
    u8 code[] = {
        /* addr */
        /*   0: */ MK_JMP, (109 - 1), 0,
        /*   3: */ MK_INC, MK_DOTS, MK_CR, MK_RET,
        /*   7: */ MK_INC, MK_U16,   3, 0, MK_CALL, MK_RET,
        /*  13: */ MK_INC, MK_U16,   7, 0, MK_CALL, MK_RET,
        /*  19: */ MK_INC, MK_U16,  13, 0, MK_CALL, MK_RET,
        /*  25: */ MK_INC, MK_U16,  19, 0, MK_CALL, MK_RET,
        /*  31: */ MK_INC, MK_U16,  25, 0, MK_CALL, MK_RET,
        /*  37: */ MK_INC, MK_U16,  31, 0, MK_CALL, MK_RET,
        /*  43: */ MK_INC, MK_U16,  37, 0, MK_CALL, MK_RET,
        /*  49: */ MK_INC, MK_U16,  43, 0, MK_CALL, MK_RET,
        /*  55: */ MK_INC, MK_U16,  49, 0, MK_CALL, MK_RET,
        /*  61: */ MK_INC, MK_U16,  55, 0, MK_CALL, MK_RET,
        /*  67: */ MK_INC, MK_U16,  61, 0, MK_CALL, MK_RET,
        /*  73: */ MK_INC, MK_U16,  67, 0, MK_CALL, MK_RET,
        /*  79: */ MK_INC, MK_U16,  73, 0, MK_CALL, MK_RET,
        /*  85: */ MK_INC, MK_U16,  79, 0, MK_CALL, MK_RET,
        /*  91: */ MK_INC, MK_U16,  85, 0, MK_CALL, MK_RET,
        /*  97: */ MK_INC, MK_U16,  91, 0, MK_CALL, MK_RET,
        /* 103: */ MK_INC, MK_U16,  97, 0, MK_CALL, MK_RET,

        /* 109: */ MK_U8, 0, MK_U8,   3, MK_CALL, MK_DROP,  /*  1 */
        /*    : */ MK_U8, 0, MK_U8,   7, MK_CALL, MK_DROP,  /*  2 */
        /*    : */ MK_U8, 0, MK_U8,  13, MK_CALL, MK_DROP,  /*  3 */
        /*    : */ MK_U8, 0, MK_U8,  19, MK_CALL, MK_DROP,  /*  4 */
        /*    : */ MK_U8, 0, MK_U8,  25, MK_CALL, MK_DROP,  /*  5 */
        /*    : */ MK_U8, 0, MK_U8,  31, MK_CALL, MK_DROP,  /*  6 */
        /*    : */ MK_U8, 0, MK_U8,  37, MK_CALL, MK_DROP,  /*  7 */
        /*    : */ MK_U8, 0, MK_U8,  43, MK_CALL, MK_DROP,  /*  8 */
        /*    : */ MK_U8, 0, MK_U8,  49, MK_CALL, MK_DROP,  /*  9 */
        /*    : */ MK_U8, 0, MK_U8,  55, MK_CALL, MK_DROP,  /*  10 */
        /*    : */ MK_U8, 0, MK_U8,  61, MK_CALL, MK_DROP,  /*  11 */
        /*    : */ MK_U8, 0, MK_U8,  67, MK_CALL, MK_DROP,  /*  12 */
        /*    : */ MK_U8, 0, MK_U8,  73, MK_CALL, MK_DROP,  /*  13 */
        /*    : */ MK_U8, 0, MK_U8,  79, MK_CALL, MK_DROP,  /*  14 */
        /*    : */ MK_U8, 0, MK_U8,  85, MK_CALL, MK_DROP,  /*  15 */
        /*    : */ MK_U8, 0, MK_U8,  91, MK_CALL, MK_DROP,  /*  16 */
        /*    : */ MK_U8, 0, MK_U8,  97, MK_CALL, MK_DROP,  /*  17 */
        /*    : */ MK_U8, 0, MK_U8, 103, MK_CALL,           /* ERROR: ... */
        MK_HALT,
    };
    char * expected =
        " 1\n"
        " 2\n"
        " 3\n"
        " 4\n"
        " 5\n"
        " 6\n"
        " 7\n"
        " 8\n"
        " 9\n"
        " 10\n"
        " 11\n"
        " 12\n"
        " 13\n"
        " 14\n"
        " 15\n"
        " 16\n"
        " 17\n"
        "ERROR: Return stack overflow\n";
    _score("test_CALL", code, expected, MK_ERR_R_OVER);
}


/* ======================================= */
/* === Memory Access: Loads and Stores === */
/* ======================================= */

/* Test LB opcode */
/* LB ( addr -- u8 ) Load u8 (byte) at address T into T as zero-filled i32. */
static void test_LB(void) {
    /* Round 1: This ends by checking for a bad address error */
    u8 code[] = {
        MK_U8, 45, MK_LB, MK_DOT, MK_CR,  /*  0 */
        MK_U8, 46, MK_LB, MK_DOT, MK_CR,  /*  5 */
        MK_U8, 47, MK_LB, MK_DOT, MK_CR,  /*  127 */
        MK_U8, 48, MK_LB, MK_DOT, MK_CR,  /*  128 */
        MK_U8, 49, MK_LB, MK_DOT, MK_CR,  /*  133 */
        MK_U8, 50, MK_LB, MK_DOT, MK_CR,  /*  255 */
        MK_I32, 255, 255, 0, 0,           /* address 65535=ffff is valid */
        MK_LB, MK_DOT, MK_CR,             /*  0 */
        MK_I32, 0, 0, 1, 0, MK_LB,        /* This will raise an error */
        MK_HALT,
        0,    /* <- address 45 */
        5,
        127,
        128,
        133,
        255,
    };
    char * expected =
        " 0\n"
        " 5\n"
        " 127\n"
        " 128\n"
        " 133\n"
        " 255\n"
        " 0\n"
        "ERROR: Bad address\n";
    _score("test_LB", code, expected, MK_ERR_BAD_ADDRESS);

    /* Round 2: This ends by checking for a stack underflow error */
    u8 code2[] = {
        MK_LB,      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        "ERROR: Stack underflow\n";
    _score("test_LB_underflow", code2, expected2, MK_ERR_D_UNDER);
}

/* Test SB opcode */
/* SB ( u8 addr -- ) Store low byte of S (u8) into address T, drop S & T. */
static void test_SB(void) {
    /* Round 1: This ends by checking for a bad address error */
    u8 code[] = {
        MK_U8,   11, MK_U8, 128, MK_SB,
        MK_U8,  's', MK_U8, 129, MK_SB,
        MK_U8,  't', MK_U8, 130, MK_SB,
        MK_U8,  'o', MK_U8, 131, MK_SB,
        MK_U8,  'r', MK_U8, 132, MK_SB,
        MK_U8,  'e', MK_U8, 133, MK_SB,
        MK_U8,  ' ', MK_U8, 134, MK_SB,
        MK_U8,  'b', MK_U8, 135, MK_SB,
        MK_U8,  'y', MK_U8, 136, MK_SB,
        MK_U8,  't', MK_U8, 137, MK_SB,
        MK_U8,  'e', MK_U8, 138, MK_SB,
        MK_U8, '\n', MK_U8, 139, MK_SB,
        MK_U8, 128, MK_PRINT,
        MK_U8, 0,
        MK_I32, 255, 255, 0, 0, MK_SB,        /* address 65535=ffff is valid */
        MK_U8, 5, MK_I32, 0, 0, 1, 0, MK_SB,  /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        "store byte\n"
        "ERROR: Bad address\n";
    _score("test_SB", code, expected, MK_ERR_BAD_ADDRESS);

    /* Round 2: This ends by checking for a stack underflow error */
    u8 code2[] = {
        MK_U8, 50,
        MK_SB,      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        "ERROR: Stack underflow\n";
    _score("test_SB_underflow", code2, expected2, MK_ERR_D_UNDER);
}

/* Test LH opcode */
/* LH ( addr -- u16 ) Load u16 (halfword) at address T, zero fill, push to T */
static void test_LH(void) {
    u8 code[] = {
        MK_U8, 40, MK_LH, MK_DOT, MK_CR,  /*  0 */
        MK_U8, 42, MK_LH, MK_DOT, MK_CR,  /*  5 */
        MK_U8, 44, MK_LH, MK_DOT, MK_CR,  /*  32767 */
        MK_U8, 46, MK_LH, MK_DOT, MK_CR,  /*  32767 */
        MK_U8, 48, MK_LH, MK_DOT, MK_CR,  /*  65535 */
        MK_I32, 254, 255, 0, 0,           /* address 65534=fffe is valid */
        MK_LH, MK_DOT, MK_CR,             /*  0 */
        MK_I32, 255, 255, 0, 0, MK_LH,    /* This will raise an error */
        MK_HALT,
          0,   0,  /* 0 */
          5,   0,  /* 5 */
        255, 127,  /* 32767 */
          0, 128,  /* 32768 */
        255, 255,  /* 65535 */
    };
    char * expected =
        " 0\n"
        " 5\n"
        " 32767\n"
        " 32768\n"
        " 65535\n"
        " 0\n"
        "ERROR: Bad address\n";
    _score("test_LH", code, expected, MK_ERR_BAD_ADDRESS);

    /* Round 2: This ends by checking for a stack underflow error */
    u8 code2[] = {
        MK_LH,      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        "ERROR: Stack underflow\n";
    _score("test_LH_underflow", code2, expected2, MK_ERR_D_UNDER);
}

/* Test SH opcode */
/* SH ( u16 addr -- ) Store low halfword of S (u16) into address T. */
static void test_SH(void) {
    u8 code[] = {
        MK_U16,  15,  's', MK_U8, 128, MK_SH,
        MK_U16, 't',  'o', MK_U8, 130, MK_SH,
        MK_U16, 'r',  'e', MK_U8, 132, MK_SH,
        MK_U16, ' ',  'h', MK_U8, 134, MK_SH,
        MK_U16, 'a',  'l', MK_U8, 136, MK_SH,
        MK_U16, 'f',  'w', MK_U8, 138, MK_SH,
        MK_U16, 'o',  'r', MK_U8, 140, MK_SH,
        MK_U16, 'd', '\n', MK_U8, 142, MK_SH,
        MK_U8, 128, MK_PRINT,
        MK_U8, 0,
        MK_I32, 254, 255, 0, 0,
        MK_DOTS, MK_CR, MK_SH,            /* address 65534=fffe is valid */
        MK_U8, 0,
        MK_I32, 255, 255, 0, 0, MK_SH,    /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        "store halfword\n"
        " 0 65534\n"
        "ERROR: Bad address\n";
    _score("test_SH", code, expected, MK_ERR_BAD_ADDRESS);

    /* Round 2: This ends by checking for a stack underflow error */
    u8 code2[] = {
        MK_U8, 50,
        MK_SH,      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        "ERROR: Stack underflow\n";
    _score("test_SH_underflow", code2, expected2, MK_ERR_D_UNDER);
}

/* Test LW opcode */
/* LW ( addr -- i32 ) Load i32 (signed word) at address T into T. */
static void test_LW(void) {
    u8 code[] = {
        MK_U8, 40, MK_LW, MK_DOT, MK_CR,  /*  0 */
        MK_U8, 44, MK_LW, MK_DOT, MK_CR,  /*  5 */
        MK_U8, 48, MK_LW, MK_DOT, MK_CR,  /*  2147483647 */
        MK_U8, 52, MK_LW, MK_DOT, MK_CR,  /*  -2147483648 */
        MK_U8, 56, MK_LW, MK_DOT, MK_CR,  /*  -1 */
        MK_I32, 252, 255, 0, 0,           /* address 65532=fffc is valid */
        MK_LW, MK_DOT, MK_CR,             /*  0 */
        MK_I32, 253, 255, 0, 0, MK_LW,    /* This will raise an error */
        MK_HALT,
          0,   0,   0,   0,
          5,   0,   0,   0,
        255, 255, 255, 127,
          0,   0,   0, 128,
        255, 255, 255, 255,
    };
    char * expected =
        " 0\n"
        " 5\n"
        " 2147483647\n"
        " -2147483648\n"
        " -1\n"
        " 0\n"
        "ERROR: Bad address\n";
    _score("test_LW", code, expected, MK_ERR_BAD_ADDRESS);

    /* Round 2: This ends by checking for a stack underflow error */
    u8 code2[] = {
        MK_LW,      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        "ERROR: Stack underflow\n";
    _score("test_LW_underflow", code2, expected2, MK_ERR_D_UNDER);
}

/* Test SW opcode */
/* SW ( u32 addr -- ) Store full word (u32) from S into address T. */
static void test_SW(void) {
    u8 code[] = {
        MK_I32,  11, 's', 't',  'o', MK_U8, 128, MK_SW,
        MK_I32, 'r', 'e', ' ',  'w', MK_U8, 132, MK_SW,
        MK_I32, 'o', 'r', 'd', '\n', MK_U8, 136, MK_SW,
        MK_U8, 128, MK_PRINT,
        MK_U8, 0,
        MK_I32, 252, 255, 0, 0,
        MK_DOTS, MK_CR, MK_SW,            /* address 65532=fffc is valid */
        MK_U8, 0,
        MK_I32, 253, 255, 0, 0, MK_SW,    /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        "store word\n"
        " 0 65532\n"
        "ERROR: Bad address\n";
    _score("test_SW", code, expected, MK_ERR_BAD_ADDRESS);

    /* Round 2: This ends by checking for a stack underflow error */
    u8 code2[] = {
        MK_U8, 50,
        MK_SW,      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        "ERROR: Stack underflow\n";
    _score("test_SW_underflow", code2, expected2, MK_ERR_D_UNDER);
}


/* ================== */
/* === Arithmetic === */
/* ================== */

/* Test INC opcode */
static void test_INC(void) {
    u8 code[] = {
        MK_U8, 0,
        MK_DOTS, MK_INC, MK_DOT, MK_CR,  /*  0 1 */
        MK_I32, 255, 255, 255, 255,
        MK_DOTS, MK_INC, MK_DOT, MK_CR,  /*  -1 0 */
        MK_I32, 255, 255, 255, 127,
        MK_DOTS, MK_INC, MK_DOT, MK_CR,  /*  2147483647 -2147483648 */
        MK_DOTS, MK_CR,                  /*  Stack is empty */
        MK_INC,                          /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 0 1\n"
        " -1 0\n"
        " 2147483647 -2147483648\n"  /* DANGER! Overflow! Beware of this! */
        " Stack is empty\n"
        "ERROR: Stack underflow\n";
    _score("test_INC", code, expected, MK_ERR_D_UNDER);
}

/* Test DEC opcode */
static void test_DEC(void) {
    u8 code[] = {
        MK_U8, 1,
        MK_DOTS, MK_DEC, MK_DOT, MK_CR,  /*  1 0 */
        MK_U8, 0,
        MK_DOTS, MK_DEC, MK_DOT, MK_CR,  /*  0 -1 */
        MK_I32, 0, 0, 0, 128,
        MK_DOTS, MK_DEC, MK_DOT, MK_CR,  /*  -2147483648 2147483647 */
        MK_DOTS, MK_CR,                  /*  Stack is empty */
        MK_DEC,                          /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 0\n"
        " 0 -1\n"
        " -2147483648 2147483647\n"  /* DANGER! Overflow! Beware of this! */
        " Stack is empty\n"
        "ERROR: Stack underflow\n";
    _score("test_DEC", code, expected, MK_ERR_D_UNDER);
}

/* Test ADD opcode */
static void test_ADD(void) {
    u8 code[] = {
        MK_U8,    0,
        MK_U8,    1,
        MK_DOTS, MK_ADD, MK_DOT, MK_CR,  /*  0 1 1 */
        MK_I32, 255, 255, 255, 255,
        MK_U8,    1,
        MK_DOTS, MK_ADD, MK_DOT, MK_CR,  /*  -1 1 0 */
        MK_U8,    5,
        MK_I32, 246, 255, 255, 255,
        MK_DOTS, MK_ADD, MK_DOT, MK_CR,  /*  5 -10 -5 */
        MK_I32, 255, 255, 255, 127,
        MK_U8,   1,
        MK_DOTS, MK_ADD, MK_DOT, MK_CR,  /*  2147483647 1 -2147483648 */
        MK_DOTS, MK_CR,                  /*  Stack is empty */
        MK_ADD,                          /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 0 1 1\n"
        " -1 1 0\n"
        " 5 -10 -5\n"                  /* Adding negative is like subtract */
        " 2147483647 1 -2147483648\n"  /* DANGER! Overflow! Beware of this! */
        " Stack is empty\n"
        "ERROR: Stack underflow\n";
    _score("test_ADD", code, expected, MK_ERR_D_UNDER);
}

/* Test SUB opcode */
static void test_SUB(void) {
    u8 code[] = {
        MK_U8,    1,
        MK_U8,    1,
        MK_DOTS, MK_SUB, MK_DOT, MK_CR,  /*  1 1 0 */
        MK_U8,    0,
        MK_U8,    1,
        MK_DOTS, MK_SUB, MK_DOT, MK_CR,  /*  0 1 -1 */
        MK_U8,    5,
        MK_I32, 246, 255, 255, 255,
        MK_DOTS, MK_SUB, MK_DOT, MK_CR,  /*  5 -10 15 */
        MK_I32,   0,   0,   0, 128,
        MK_U8,    1,
        MK_DOTS, MK_SUB, MK_DOT, MK_CR,  /*  -2147483648 1 2147483647 */
        MK_DOTS, MK_CR,                  /*  Stack is empty */
        MK_SUB,                          /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 1 0\n"
        " 0 1 -1\n"
        " 5 -10 15\n"                  /* Subracting a negative is like add */
        " -2147483648 1 2147483647\n"  /* DANGER! Overflow! Beware of this! */
        " Stack is empty\n"
        "ERROR: Stack underflow\n";
    _score("test_SUB", code, expected, MK_ERR_D_UNDER);
}

/* Test MUL opcode */
static void test_MUL(void) {
    u8 code[] = {
        MK_U8,    1,
        MK_U8,    1,
        MK_DOTS, MK_MUL, MK_DOT, MK_CR,  /*  1 1 1 */
        MK_U8,    1,
        MK_U8,    0,
        MK_DOTS, MK_MUL, MK_DOT, MK_CR,  /*  1 0 0 */
        MK_U8,    5,
        MK_U8,    5,
        MK_DOTS, MK_MUL, MK_DOT, MK_CR,  /*  5 5 25 */
        MK_U8,    5,
        MK_I32, 246, 255, 255, 255,
        MK_DOTS, MK_MUL, MK_DOT, MK_CR,  /*  5 -10 -50 */
        MK_I32, 246, 255, 255, 255,
        MK_I32, 246, 255, 255, 255,
        MK_DOTS, MK_MUL, MK_DOT, MK_CR,  /*  -10 -10 100 */
        MK_I32, 255, 255, 255, 127,
        MK_I32, 255, 255, 255, 127,
        MK_DOTS, MK_MUL, MK_DOT, MK_CR,  /*  2147483647 2147483647 1 */
        MK_I32,   0,   0,   0, 128,
        MK_I32,   0,   0,   0, 128,
        MK_DOTS, MK_MUL, MK_DOT, MK_CR,  /*  -2147483648 -2147483648 0 */
        MK_U8,    1,
        MK_DOTS, MK_CR,                  /*  1 */
        MK_MUL,                          /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 1 1\n"
        " 1 0 0\n"
        " 5 5 25\n"
        " 5 -10 -50\n"                  /* (+) * (-) = (-) */
        " -10 -10 100\n"                /* (-) * (-) = (+) */
        " 2147483647 2147483647 1\n"    /* DANGER! Overflow! Beware of this! */
        " -2147483648 -2147483648 0\n"  /* DANGER! Overflow! Beware of this! */
        " 1\n"
        "ERROR: Stack underflow\n";
    _score("test_MUL", code, expected, MK_ERR_D_UNDER);
}

/* Test DIV opcode */
/* CAUTION! Some divisor / dividend combinations can cause hardware traps! */
/* CAUTION! Divide by zero is bad, but so is -2147483648 / -1.             */
/*
 * Notes on testing dividends and divisors on macOS with this ANSI C test code:
 *     uint32_t a = ...;
 *     uint32_t b = ...;
 *     uint32_t c = a / b;
 *     printf("%d / %d = %d, a, b, c);
 *
 * Results:
 *     -2147483647 / -1 = 2147483647
 *     -2147483648 / -2147483648 = 1
 *     -2147483648 / -1 => process exits with "floating point exception"
 *       anything  /  0 => process exits with "floating point exception"
 *
 * The problem with -2147483648 / -1 is that, if implemented with an amd64
 * opcode for 32-bit signed division, the quotient will overflow. 2147483648
 * fits in uint32_t, but not in int32_t. The quotient overflow triggers a CPU
 * hardware trap, similar to what happens with a divide by zero. Allowing the
 * hardware trap to happen would result in the process for the markab bytecode
 * interpreter getting killed by the OS. That would be _BAD_, so we can't let
 * that happen.
 */
static void test_DIV(void) {
    /* Round 1: This ends by checking for a divide by zero error */
    u8 code[] = {
        MK_U8,    1,
        MK_U8,    1,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  1 1 1 */
        MK_U8,    0,
        MK_U8,    1,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  0 1 0 */
        MK_U8,   25,
        MK_U8,    5,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  25 5 5 */
        MK_I32, 246, 255, 255, 255,
        MK_U8,    5,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  -10 5 -2 */
        MK_I32, 246, 255, 255, 255,
        MK_I32, 251, 255, 255, 255,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  -10 -5 2 */
        MK_U8,   50,
        MK_I32, 246, 255, 255, 255,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  50 -10 -5 */
        MK_I32, 255, 255, 255, 127,
        MK_I32, 255, 255, 255, 127,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  2147483647 2147483647 1 */
        MK_I32,   0,   0,   0, 128,
        MK_I32,   0,   0,   0, 128,
        MK_DOTS, MK_DIV, MK_DOT, MK_CR,  /*  -2147483648 -2147483648 1 */
        MK_U8,    1,
        MK_U8,    0,
        MK_DOTS, MK_CR,                 /* 1 0 */
        MK_DIV,                         /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 1 1\n"
        " 0 1 0\n"
        " 25 5 5\n"
        " -10 5 -2\n"
        " -10 -5 2\n"
        " 50 -10 -5\n"
        " 2147483647 2147483647 1\n"
        " -2147483648 -2147483648 1\n"
        " 1 0\n"
        "ERROR: Divide by zero\n";
    _score("test_DIV", code, expected, MK_ERR_DIV_BY_ZERO);

    /* Round 2: This ends by checking for a quotient overflow error */
    u8 code2[] = {
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 255,
        MK_DOTS, MK_CR,              /*  -2147483648 -1 */
        MK_DIV,                      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        " -2147483648 -1\n"
        "ERROR: Quotient would overflow\n";
    _score("test_DIV_overflow", code2, expected2, MK_ERR_DIV_OVERFLOW);
}

/* Test MOD opcode */
/* CAUTION! Some divisor / dividend combinations can cause hardware traps! */
/* CAUTION! Divide by zero is bad, but so is -2147483648 % -1.             */
static void test_MOD(void) {
    /* Round 1: This ends by checking for a divide by zero error */
    u8 code[] = {
        MK_U8,    1,
        MK_U8,    1,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  1 1 0 */
        MK_U8,    0,
        MK_U8,    1,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  0 1 0 */
        MK_U8,   25,
        MK_U8,    3,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  25 3 1 */
        MK_I32, 246, 255, 255, 255,
        MK_U8,    3,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  -10 3 -1 */
        MK_I32, 246, 255, 255, 255,
        MK_I32, 253, 255, 255, 255,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  -10 -3 -1 */
        MK_U8,   50,
        MK_I32, 248, 255, 255, 255,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  50 -8 2 */
        MK_I32, 255, 255, 255, 127,
        MK_I32, 255, 255, 255, 127,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  2147483647 2147483647 0 */
        MK_I32,   0,   0,   0, 128,
        MK_I32,   0,   0,   0, 128,
        MK_DOTS, MK_MOD, MK_DOT, MK_CR,  /*  -2147483648 -2147483648 0 */
        MK_U8,    1,
        MK_U8,    0,
        MK_DOTS, MK_CR,                 /* 1 0 */
        MK_MOD,                         /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 1 0\n"
        " 0 1 0\n"
        " 25 3 1\n"
        " -10 3 -1\n"
        " -10 -3 -1\n"
        " 50 -8 2\n"
        " 2147483647 2147483647 0\n"
        " -2147483648 -2147483648 0\n"
        " 1 0\n"
        "ERROR: Divide by zero\n";
    _score("test_MOD", code, expected, MK_ERR_DIV_BY_ZERO);

    /* Round 2: This ends by checking for a quotient overflow error */
    u8 code2[] = {
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 255,
        MK_DOTS, MK_CR,              /*  -2147483648 -1 */
        MK_MOD,                      /* This will raise an error */
        MK_HALT,
    };
    char * expected2 =
        " -2147483648 -1\n"
        "ERROR: Quotient would overflow\n";
    _score("test_MOD_overflow", code2, expected2, MK_ERR_DIV_OVERFLOW);
}


/* ============== */
/* === Shifts === */
/* ============== */

/* Test SLL opcode */
static void test_SLL(void) {
    u8 code[] = {
        MK_HEX,
        MK_U8, 1,
        MK_U8, 31,
        MK_DOTSH, MK_SLL, MK_DOT, MK_CR,  /*  1 1e 80000000 */
        MK_DECIMAL,
        MK_U8, 5,
        MK_U8, 2,
        MK_DOTSH, MK_SLL, MK_DOT, MK_CR,  /*  5 2 20 */
        MK_SLL,                           /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 1f 80000000\n"
        " 5 2 20\n"         /* 5*pow(2,2) = 5*4 = 20 */
        "ERROR: Stack underflow\n";
    _score("test_SLL", code, expected, MK_ERR_D_UNDER);
}

/* Test SRL opcode */
static void test_SRL(void) {
    u8 code[] = {
        MK_HEX,
        MK_I32, 255, 255, 255, 255,
        MK_U8, 30,
        MK_DOTSH, MK_SRL, MK_DOT, MK_CR,  /*  ffffffff 1e 3 */
        MK_I32, 255, 255, 255, 127,
        MK_U8, 30,
        MK_DOTSH, MK_SRL, MK_DOT, MK_CR,  /*  7fffffff 1e 1 */
        MK_U8, 20,
        MK_U8, 2,
        MK_DECIMAL,
        MK_DOTS,  MK_SRL, MK_DOT, MK_CR,  /*  20 2 5 */
        MK_SRL,                           /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " ffffffff 1e 3\n"  /* sign bit is 1, fill is 0 */
        " 7fffffff 1e 1\n"  /* sign bit is 0, fill is 0 */
        " 20 2 5\n"         /* 20/pow(2,2) = 20/4 = 5 */
        "ERROR: Stack underflow\n";
    _score("test_SRL", code, expected, MK_ERR_D_UNDER);
}

/* Test SRA opcode */
static void test_SRA(void) {
    u8 code[] = {
        MK_HEX,
        MK_I32, 255, 255, 255, 255,
        MK_U8, 30,
        MK_DOTSH, MK_SRA, MK_DOT, MK_CR,  /*  ffffffff 1e ffffffff */
        MK_I32, 255, 255, 255, 127,
        MK_U8, 30,
        MK_DOTSH, MK_SRA, MK_DOT, MK_CR,  /*  7fffffff 1e 1 */
        MK_U8, 20,
        MK_U8, 2,
        MK_DECIMAL,
        MK_DOTS,  MK_SRA, MK_DOT, MK_CR,  /*  20 2 5 */
        MK_SRA,                           /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " ffffffff 1e ffffffff\n"  /* sign bit is 1, fill is 1 */
        " 7fffffff 1e 1\n"         /* sign bit is 0, fill is 0 */
        " 20 2 5\n"                /* 20/pow(2,2) = 20/4 = 5 */
        "ERROR: Stack underflow\n";
    _score("test_SRA", code, expected, MK_ERR_D_UNDER);
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
        MK_U8, 0, MK_U8, 0, MK_DOTS, MK_CR,  /*  0 0 */
        MK_GT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_U8, 0, MK_U8, 1, MK_DOTS, MK_CR,  /*  0 1 */
        MK_GT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_U8, 0, MK_TRUE,  MK_DOTS, MK_CR,  /*  0 -1 */
        MK_GT,              MK_DOTS, MK_CR,  /*  -1 */
        MK_DROP,
        MK_TRUE,  MK_TRUE,  MK_DOTS, MK_CR,  /*  -1 -1 */
        MK_GT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_TRUE, MK_U8, 0,  MK_DOTS, MK_CR,  /*  -1 0 */
        MK_GT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_I32, 255, 255, 255, 127,
        MK_I32,   0,   0,   0, 128,
        MK_DOTS, MK_CR,                      /*  2147483647 -2147483648 */
        MK_GT,              MK_DOTS, MK_CR,  /*  -1 */
        MK_DROP,
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 127,
        MK_DOTS, MK_CR,                      /*  -2147483648 2147483647 */
        MK_GT,              MK_DOTS, MK_CR,  /*  0 */
        MK_GT,                               /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 0 0\n"
        " 0\n"
        " 0 1\n"
        " 0\n"
        " 0 -1\n"
        " -1\n"
        " -1 -1\n"
        " 0\n"
        " -1 0\n"
        " 0\n"
        " 2147483647 -2147483648\n"
        " -1\n"
        " -2147483648 2147483647\n"
        " 0\n"
        "ERROR: Stack underflow\n";
    _score("test_GT", code, expected, MK_ERR_D_UNDER);
}

/* Test LT opcode */
static void test_LT(void) {
    u8 code[] = {
        MK_U8, 0, MK_U8, 0, MK_DOTS, MK_CR,  /*  0 0 */
        MK_LT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_U8, 0, MK_U8, 1, MK_DOTS, MK_CR,  /*  0 1 */
        MK_LT,              MK_DOTS, MK_CR,  /*  -1 */
        MK_DROP,
        MK_U8, 0, MK_TRUE,  MK_DOTS, MK_CR,  /*  0 -1 */
        MK_LT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_TRUE,  MK_TRUE,  MK_DOTS, MK_CR,  /*  -1 -1 */
        MK_LT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_TRUE, MK_U8, 0,  MK_DOTS, MK_CR,  /*  -1 0 */
        MK_LT,              MK_DOTS, MK_CR,  /*  -1 */
        MK_DROP,
        MK_I32, 255, 255, 255, 127,
        MK_I32,   0,   0,   0, 128,
        MK_DOTS, MK_CR,                      /*  2147483647 -2147483648 */
        MK_LT,              MK_DOTS, MK_CR,  /*  0 */
        MK_DROP,
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 127,
        MK_DOTS, MK_CR,                      /*  -2147483648 2147483647 */
        MK_LT,              MK_DOTS, MK_CR,  /*  -1 */
        MK_LT,                               /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 0 0\n"
        " 0\n"
        " 0 1\n"
        " -1\n"
        " 0 -1\n"
        " 0\n"
        " -1 -1\n"
        " 0\n"
        " -1 0\n"
        " -1\n"
        " 2147483647 -2147483648\n"
        " 0\n"
        " -2147483648 2147483647\n"
        " -1\n"
        "ERROR: Stack underflow\n";
    _score("test_LT", code, expected, MK_ERR_D_UNDER);
}

/* Test EQ opcode */
static void test_EQ(void) {
    u8 code[] = {
        MK_TRUE,   MK_TRUE, MK_DOTS, MK_EQ, MK_DOT, MK_CR,  /*  -1 -1 -1 */
        MK_TRUE,  MK_FALSE, MK_DOTS, MK_EQ, MK_DOT, MK_CR,  /*  -1 0 0 */
        MK_FALSE,  MK_TRUE, MK_DOTS, MK_EQ, MK_DOT, MK_CR,  /*  0 -1 0 */
        MK_FALSE, MK_FALSE, MK_DOTS, MK_EQ, MK_DOT, MK_CR,  /*  0 0 -1 */
        MK_U8, 0, MK_U8, 5, MK_DOTS, MK_EQ, MK_DOT, MK_CR,  /*  0 5 0 */
        MK_U8, 5, MK_U8, 5, MK_DOTS, MK_EQ, MK_DOT, MK_CR,  /*  5 5 -1 */
        MK_EQ,                                /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " -1 -1 -1\n"
        " -1 0 0\n"
        " 0 -1 0\n"
        " 0 0 -1\n"
        " 0 5 0\n"
        " 5 5 -1\n"
        "ERROR: Stack underflow\n";
    _score("test_EQ", code, expected, MK_ERR_D_UNDER);
}

/* Test NE opcode */
static void test_NE(void) {
    u8 code[] = {
        MK_TRUE,   MK_TRUE, MK_DOTS, MK_NE, MK_DOT, MK_CR,  /*  -1 -1 0 */
        MK_TRUE,  MK_FALSE, MK_DOTS, MK_NE, MK_DOT, MK_CR,  /*  -1 0 -1 */
        MK_FALSE,  MK_TRUE, MK_DOTS, MK_NE, MK_DOT, MK_CR,  /*  0 -1 -1 */
        MK_FALSE, MK_FALSE, MK_DOTS, MK_NE, MK_DOT, MK_CR,  /*  0 0 0 */
        MK_U8, 0, MK_U8, 5, MK_DOTS, MK_NE, MK_DOT, MK_CR,  /*  0 5 -1 */
        MK_U8, 5, MK_U8, 5, MK_DOTS, MK_NE, MK_DOT, MK_CR,  /*  5 5 0 */
        MK_NE,                                /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " -1 -1 0\n"
        " -1 0 -1\n"
        " 0 -1 -1\n"
        " 0 0 0\n"
        " 0 5 -1\n"
        " 5 5 0\n"
        "ERROR: Stack underflow\n";
    _score("test_NE", code, expected, MK_ERR_D_UNDER);
}

/* Test ZE opcode */
static void test_ZE(void) {
    u8 code[] = {
        MK_U8, 0, MK_DOTS, MK_ZE, MK_DOT, MK_CR,  /*  0 -1 */
        MK_TRUE,  MK_DOTS, MK_ZE, MK_DOT, MK_CR,  /*  -1 0 */
        MK_U8, 1, MK_DOTS, MK_ZE, MK_DOT, MK_CR,  /*  1 0 */
        MK_U8, 5, MK_DOTS, MK_ZE, MK_DOT, MK_CR,  /*  5 0 */
        MK_ZE,                                 /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 0 -1\n"
        " -1 0\n"
        " 1 0\n"
        " 5 0\n"
        "ERROR: Stack underflow\n";
    _score("test_ZE", code, expected, MK_ERR_D_UNDER);
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


/* ======================== */
/* === Error Conditions === */
/* ======================== */

/* Test ERR_OK (error code indicating no errors) */
static void test_ERR_OK(void) {
    u8 code[] = {
        MK_STR, 3, 'O', 'K', '\n', MK_PRINT,
        MK_HALT,
    };
    char * expected = "OK\n";
    _score("test_ERR_OK", code, expected, MK_ERR_OK);
}

/* Test ERR_D_OVER error (stack overflow) */
static void test_ERR_D_OVER(void) {
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
        MK_U8, 19, MK_DOTSH, MK_CR,
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
        "ERROR: Stack overflow\n";
    _score("test_ERR_D_OVER", code, expected, MK_ERR_D_OVER);
}

/* Test ERR_D_UNDER error (stack underflow) */
static void test_ERR_D_UNDER(void) {
    u8 code[] = {
        MK_DUP,
        MK_HALT,
    };
    char * expected = "ERROR: Stack underflow\n";
    _score("test_ERR_D_UNDER", code, expected, MK_ERR_D_UNDER);
}

/* Test ERR_R_OVER error (return stack overflow) */
static void test_ERR_R_OVER(void) {
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
        MK_U8, 18, MK_MTR, MK_DOTRH, MK_CR,
        MK_U8, 19, MK_MTR, MK_DOTRH, MK_CR,
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
        "ERROR: Return stack overflow\n";
    _score("test_ERR_R_OVER", code, expected, MK_ERR_R_OVER);
}

/* Test ERR_R_UNDER error (return stack underflow */
static void test_ERR_R_UNDER(void) {
    u8 code[] = {
        MK_RDROP,
        MK_HALT,
    };
    char * expected = "ERROR: Return stack underflow\n";
    _score("test_ERR_R_UNDER", code, expected, MK_ERR_R_UNDER);
}

/* Test ERR_BAD_ADDRESS error */
static void test_ERR_BAD_ADDRESS(void) {
    u8 code[] = {
        MK_I32, 0, 0, 1, 0, MK_LB,  /* 65536 = 0x00010000 */
        MK_HALT,
    };
    char * expected = "ERROR: Bad address\n";
    _score("test_ERR_BAD_ADDRESS", code, expected, MK_ERR_BAD_ADDRESS);
}

/* Test ERR_BAD_OPCODE error */
static void test_ERR_BAD_OPCODE(void) {
    u8 code[] = {
        255,
        MK_HALT,
    };
    char * expected = "ERROR: Bad opcode\n";
    _score("test_ERR_BAD_OPCODE", code, expected, MK_ERR_BAD_OPCODE);
}

/* Test ERR_CPU_HOG error (code was hogging CPU) */
static void test_ERR_CPU_HOG(void) {
    /* This loop attempts to run forever. */
    u8 code[] = {
        MK_U8, 0,                      /* 1: initialize counter to zero     */
        MK_DUP, MK_U16, 0, 2, MK_MOD,  /* 2: compute counter % 512          */
        MK_ZE, MK_BZ, 3,               /* 3: if result != 0, jump to line 5 */
        MK_DOTS, MK_CR,                /* 4:   print the counter            */
        MK_INC,                        /* 5: increment counter              */
        MK_JMP, 244, 255,              /* 6: jump back to line 2 (-12 ops)  */
        MK_HALT,
    };
    char * expected =
        " 0\n"
        " 512\n"
        " 1024\n"
        " 1536\n"
        " 2048\n"
        " 2560\n"
        " 3072\n"
        " 3584\n"
        " 4096\n"
        " 4608\n"
        " 5120\n"
        " 5632\n"
        " 6144\n"
        " 6656\n"
        " 7168\n"
        " 7680\n"
        " 8192\n"
        " 8704\n"
        " 9216\n"
        "ERROR: Code was hogging CPU\n";
    _score("test_ERR_CPU_HOG", code, expected, MK_ERR_CPU_HOG);
}

/* Test ERR_DIV_BY_ZERO error */
static void test_ERR_DIV_BY_ZERO(void) {
    u8 code[] = {
        MK_U8, 1,
        MK_U8, 0,
        MK_DOTS, MK_CR,                 /* 1 0 */
        MK_DIV,                         /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " 1 0\n"
        "ERROR: Divide by zero\n";
    _score("test_ERR_DIV_BY_ZERO", code, expected, MK_ERR_DIV_BY_ZERO);
}

/* Test ERR_DIV_OVERFLOW error */
static void test_ERR_DIV_OVERFLOW(void) {
    u8 code[] = {
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 255,
        MK_DOTS, MK_CR,              /*  -2147483648 -1 */
        MK_DIV,                      /* This will raise an error */
        MK_HALT,
    };
    char * expected =
        " -2147483648 -1\n"
        "ERROR: Quotient would overflow\n";
    _score("test_ERR_DIV_OVERFLOW", code, expected, MK_ERR_DIV_OVERFLOW);
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

    /*  Error Conditions */
    test_ERR_OK();
    test_ERR_D_OVER();
    test_ERR_D_UNDER();
    test_ERR_R_OVER();
    test_ERR_R_UNDER();
    test_ERR_BAD_ADDRESS();
    test_ERR_BAD_OPCODE();
    test_ERR_CPU_HOG();
    test_ERR_DIV_BY_ZERO();
    test_ERR_DIV_OVERFLOW();

    /* If any tests failed, print the failed test log */
    if(TEST_SCORE_FAIL > 0) {
        char * fail_header =
            "[=========================]\n"
            "[ List of Failing Tests   ]\n";
        char * fail_footer = "[=========================]\n";
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

