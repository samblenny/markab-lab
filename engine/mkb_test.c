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
#  include <stdint.h>         /* uint8_t, uint16_t, int32_t, ... */
#  include <stdio.h>          /* printf(), putchar(), ... */
#  include <string.h>         /* memset() */
#  include <unistd.h>         /* STDOUT_FILENO */
#endif
#include "libmkb/libmkb.h"
#include "libmkb/autogen.h"


/* ========================
 * Test scoring global vars
 * ========================
 */

/* Typedef for a jumbo-size counted string buffer */
#define MKB_TEST_StrBufSize (65536)
typedef struct mkb_test_str {
    u16 len;
    u8 buf[MKB_TEST_StrBufSize];
} mkb_test_str_t;

/* Global buffer used for capturing the VM's writes to stdout during tests */
static mkb_test_str_t TEST_STDOUT;

/* Global var for counting passed tests */
static int TEST_SCORE_PASS = 0;

/* Global var for counting failed tests */
static int TEST_SCORE_FAIL = 0;


/* ==============================
 * Test scoring utility functions
 * ==============================
 */

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
#ifdef PLAN_9
    print(fmt, name);
#else
    printf(fmt, name);
#endif
}


/* ========================================
 * Libmkb Host API implementation functions
 * ========================================
 */

/* Write an error code to stderr */
void mk_host_log_error(u8 error_code) {
    const char * fmt = "mk_host_log_code(%d)\n";
#ifdef PLAN_9
    print(fmt, error_code);
#else
    printf(fmt, error_code);
#endif
}

/* Write length bytes from byte buffer buf to real stdout and TEST_STDOUT */
void mk_host_stdout_write(const void * buf, int length) {
    /* First write to real stdout */
#ifdef PLAN_9
    write(1 /* STDOUT */, buf, length);
#else
    write(STDOUT_FILENO, buf, length);
#endif
    /* Then append a copy to the TEST_STDOUT */
    if(TEST_STDOUT.len + length < MKB_TEST_StrBufSize) {
        memcpy((void *)&(TEST_STDOUT.buf[TEST_STDOUT.len]), buf, length);
        TEST_STDOUT.len += length;
    }
}

/* Write byte to stdout and TEST_STDOUT */
void mk_host_putchar(u8 data) {
    /* First write to real stdout */
    putchar(data);
    /* Then append a copy to the TEST_STDOUT */
    if(TEST_STDOUT.len + 1 < MKB_TEST_StrBufSize) {
        TEST_STDOUT.buf[TEST_STDOUT.len] = data;
        TEST_STDOUT.len += 1;
    }
}


/* ===========================================
 * And, finally... the actual tests start here
 * ===========================================
 */

#define TEST_ROM_SIZE (300)

/* Macro: run code, check expected output, score results, reset TEST_STDOUT */
#define _score(NAME, CODE, EXPECT_S, EXPECT_E) {        \
    if(EXPECT_E != mk_load_rom(CODE, TEST_ROM_SIZE)) {  \
        score_fail(NAME);                               \
    } else {                                            \
        if(test_stdout_match(EXPECT_S)) {               \
            score_pass(NAME);                           \
        } else {                                        \
            score_fail(NAME);                           \
        }                                               \
    }                                                   \
    test_stdout_reset();                                }

/* Test NOP opcode */
static void test_NOP(void) {
    u8 code[TEST_ROM_SIZE] = {
        MK_NOP, MK_PC, MK_DOTS,
        MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected = " 2\n";
    _score("test_NOP", code, expected, MK_ERR_OK);
}

/* Test DUMP opcode */
static void test_DUMP(void) {
    u8 n = 128;
    u8 code[TEST_ROM_SIZE] = {
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

/* Test EMIT opcode */
static void test_EMIT(void) {
    u8 code[TEST_ROM_SIZE] = {
        MK_U8, 'E', MK_EMIT,
        MK_U8, 'm', MK_EMIT,
        MK_U8, 'i', MK_EMIT,
        MK_U8, 't', MK_EMIT,
        MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected = "Emit\n";
    _score("test_EMIT", code, expected, MK_ERR_OK);
}

/* Test DOTRH opcode */
static void test_DOTRH(void) {
    u8 code[TEST_ROM_SIZE] = {
        MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_MTR, MK_DOTRH, MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected =
        " R-Stack is empty\n"
        " 1f0f\n"
        " 1f0f 1\n"
        " 1f0f 1 1abcdef\n"
        " 1f0f 1 1abcdef e1abcdef\n";
    _score("test_DOTRH", code, expected, MK_ERR_OK);
}

/* Test DOTS opcode */
static void test_DOTS(void) {
    u8 code[TEST_ROM_SIZE] = {
        MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_DOTS, MK_U8, '\n', MK_EMIT,
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
    u8 code[TEST_ROM_SIZE] = {
        MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x0f, 0x1f, 0x00, 0x00, MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0x01, 0x00, 0x00, 0x00, MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0x01, MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_I32, 0xef, 0xcd, 0xab, 0xe1, MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected =
        " Stack is empty\n"
        " 1f0f\n"
        " 1f0f 1\n"
        " 1f0f 1 1abcdef\n"
        " 1f0f 1 1abcdef e1abcdef\n";
    _score("test_DOTSH", code, expected, MK_ERR_OK);
}

/* Test U8 opcode */
static void test_U8(void) {
    u8 code[TEST_ROM_SIZE] = {
        MK_U8, 0, MK_U8, 1, MK_U8, 127, MK_U8, 128, MK_U8, 255,
        MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected = " 0 1 127 128 255\n";
    _score("test_U8", code, expected, MK_ERR_OK);
}

/* Test U16 opcode */
static void test_U16(void) {
    u8 code[TEST_ROM_SIZE] = {
        MK_U16,   0,   0,
        MK_U16,   1,   0,
        MK_U16, 127,   0,
        MK_U16, 128,   0,
        MK_U16, 255,   0,
        MK_U16,   0,   1,
        MK_U16,   0, 127,
        MK_U16,   0, 128,
        MK_U16, 255, 255,
        MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected = " 0 1 127 128 255 256 32512 32768 65535\n";
    _score("test_U16", code, expected, MK_ERR_OK);
}

/* Test I32 opcode */
static void test_I32(void) {
    u8 code[TEST_ROM_SIZE] = {
        MK_I32,   0,   0,   0,   0,
        MK_I32,   1,   0,   0,   0,
        MK_I32, 255, 255, 255, 127,
        MK_I32,   0,   0,   0, 128,
        MK_I32, 255, 255, 255, 255,
        MK_DOTSH, MK_U8, '\n', MK_EMIT,
        MK_DOTS, MK_U8, '\n', MK_EMIT,
        MK_HALT,
    };
    char * expected =
        " 0 1 7fffffff 80000000 ffffffff\n"
        " 0 1 2147483647 -2147483648 -1\n";
    _score("test_I32", code, expected, MK_ERR_OK);
}

/* Test opcode
static void test_() {
    u8 code[TEST_ROM_SIZE] = {
        MK_HALT,
    };
    char * expected =
        ;
    _score("test_", code, expected, MK_ERR_OK);
}
*/


/* ===================================================
 * == main() =========================================
 * ===================================================
 */

#ifdef PLAN_9
void main() {
#else
int main() {
#endif
    /* Clear the buffer used to capture the VM's stdout writes during tests */
    test_stdout_reset();
    /* Run opcode tests */
    test_NOP();
    test_DUMP();
    test_EMIT();
    test_DOTRH();
    test_DOTS();
    test_DOTSH();
    test_U8();
    test_U16();
    test_I32();
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

