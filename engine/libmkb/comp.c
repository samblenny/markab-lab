/* Copyright (c) 2023 Sam Blenny
 * SPDX-License-Identifier: MIT
 *
 * Compile source code to bytecode for the Markab bytecode interpreter.
 */
#ifndef LIBMKB_COMP_C
#define LIBMKB_COMP_C

#include "libmkb.h"
#include "autogen.h"
#include "comp.h"


/* ================================ */
/* == Typedefs, Enums, Constants == */
/* ================================ */

/* Typedef for context struct to track compiler's lexing and parsing state */
typedef struct comp_context {
    const u8 * buf;
    u32 len;
    u32 lineNum;
    u32 lineStart;
    u32 cursor;
    u32 wordEnd;
} comp_context_t;

/* Compiler error status codes */
typedef enum {
    stat_OK,             /* Success                              */
    stat_BadContext,     /* Compiler context struct is corrupted */
    stat_EOF,            /* End of input                         */
    stat_OutOfMemory,    /* VM RAM image is full                 */
    stat_ParserError,    /* Parser error: malformed literal, etc */
    stat_IntSyntax,      /* Bad int literal (syntax?)            */
    stat_IntOutOfRange,  /* Int literal is out of i32 range      */
    stat_StrSyntax,      /* Bad string literal                   */
} comp_stat;


/* ============ */
/* == Macros == */
/* ============ */

/* Macro: Make sure the compiler context hasn't gotten corrupted        */
/* CAUTION! This will cause enclosing function to return if check fails */
#define _assert_valid_comp_context()              \
    if( (comp_ctx->cursor > comp_ctx->wordEnd) |  \
        (comp_ctx->wordEnd >= comp_ctx->len)      \
    ) {                                           \
        return stat_BadContext;                   \
    }

/* Macro to compute pointer to start of word from compiler context */
#define _word_pointer(CTX) (&(CTX)->buf[(CTX)->cursor])

/* Macro to compute length of word from compiler context */
#define _word_length(CTX) ((CTX)->wordEnd - (CTX)->cursor + 1)

/* Macro: Make sure VM context's dictionary can accomodate N more bytes */
/* CAUTION! This will cause enclosing function to return if check fails */
#define _assert_dictionary_free_space(N)  \
    if((u32)ctx->DP + N >= MK_HEAP_MAX) {  \
        return stat_OutOfMemory;          \
    }

/* Macro: Append a byte to the dictionary in the VM context's RAM */
#define _append_dictionary_byte(B) {  \
    ctx->RAM[ctx->DP] = (B);          \
    ctx->DP += 1;                     }

/* Advance the lexing cursor by N bytes */
#define _advance_cursor(N)                        \
    if(comp_ctx->cursor + (N) < comp_ctx->len) {  \
        comp_ctx->cursor += (N);                  \
    }

/* Advance the wordEnd index to match the lexing cursor */
#define _sync_wordEnd_to_cursor() { comp_ctx->wordEnd = comp_ctx->cursor; }


/* ======================= */
/* == Utility Functions == */
/* ======================= */

/* Log compiler error using the libmkb Host API */
void log_compiler_error(comp_context_t * comp_ctx, comp_stat status) {
    const char * str1 = "CompileError:";
    const char * str2 = ":";
    const char * str3 = ": ";
    const char * strCR = "\n";
    /* Print the error message with line and column numbers */
    mk_host_stdout_write(str1, strlen(str1));
    const int line = comp_ctx->lineNum;
    const int lineStart = comp_ctx->lineStart;
    const int cursor = comp_ctx->cursor;
    int column = cursor - lineStart + 1;
    if(lineStart + column + 1 >= comp_ctx->len) {
        column = comp_ctx->len - 1 - lineStart;
    }
    if(lineStart + column < 0) {
        column = 0;
    }
    mk_host_stdout_fmt_int(line);
    mk_host_stdout_write(str2, strlen(str2));
    mk_host_stdout_fmt_int(column);
    mk_host_stdout_write(str3, strlen(str3));
    char * message = "??? Unknown ???";
    switch(status) {
        case stat_OK:
            message = "OK";
            break;
        case stat_BadContext:
            message = "BadContext";
            break;
        case stat_EOF:
            message = "EOF";
            break;
        case stat_OutOfMemory:
            message = "OutOfMemory";
            break;
        case stat_ParserError:
            message = "ParserError";
            break;
        case stat_IntSyntax:
            message = "IntSyntax";
            break;
        case stat_IntOutOfRange:
            message = "IntOutOfRange";
            break;
        case stat_StrSyntax:
            message = "StrSyntax";
            break;
    }
    mk_host_stdout_write(message, strlen(message));
    /* Now print the offending input text */
    mk_host_stdout_write(strCR, strlen(strCR));
    mk_host_stdout_write(&comp_ctx->buf[comp_ctx->lineStart], column + 1);
    mk_host_stdout_write(strCR, strlen(strCR));
    /* And add a caret below the offending column */
    char pad[128];
    if((1 <= column) && (column + 1 < sizeof(pad))) {
        memset(pad, ' ', sizeof(pad));
        pad[column] = '^';
        pad[column + 1] = 0;
        mk_host_stdout_write(pad, strlen(pad));
        mk_host_stdout_write(strCR, strlen(strCR));
    }
}

/* Hash a name string into a bin offset using multiply-with-carry (MWC) hash */
static comp_stat
hash_name(comp_context_t * comp_ctx, u16 * hash_bin_offset) {
    _assert_valid_comp_context();
    /* Calculate name's start position and length within the input buffer */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    /* Calculate the hashmap bin */
    int i;
    u16 k = MK_comp_HashC;
    for(i = 0; i < length; i++) {
        /* Step the multiply-with-carry PRNG */
        k = (k << MK_comp_HashA) + (k >> 16);
        /* Mix the RNG output with a byte from the string */
        k ^= buf[i];
    }
    k ^= k >> MK_comp_HashB;    /* Compress entropy towards low bits  */
    k &= MK_comp_HashMask;      /* Mask low bits to get a hashmap bin */
    *hash_bin_offset = k << 1;  /* Bin number * 2 = Bin pointer       */
    return stat_OK;             /* Success */
}


/* =========== */
/* == Lexer == */
/* =========== */

/* Advance the cursor to skip whitespace */
static comp_stat
lex_skip_whitespace(comp_context_t * comp_ctx) {
    _assert_valid_comp_context();
    /* Compute the range of ctx->buf indexes to search within */
    u32 end = comp_ctx->len - 1;
    u32 start = comp_ctx->cursor;
    /* Skip whitespace! */
    u32 i;
    u8 done = 0;
    for(i = start; i <= end && !done; i++) {
        /* Always advance the cursor */
        /* Update the line tracker and look for non-whitespace */
        switch(comp_ctx->buf[i]) {
            /* For space, tab, NULL, or a CR that is probably the start */
            /* of a CR/LF Windows-style line ending, just keep looping  */
            case 0:
            case ' ':
            case '\t':
            case '\r':
                comp_ctx->cursor = (i < end) ? i + 1 : end;
                break;
            /* For LF newlines, update the line tracker */
            case '\n':
                if(i < end) {
                    /* Only advance the line tracker if there's more input */
                    comp_ctx->cursor = i + 1;
                    comp_ctx->lineStart = i + 1;
                    comp_ctx->lineNum += 1;
                }
                break;
            /* non-whitespace means we're done */
            default:
                comp_ctx->cursor = i;
                done = 1;
        }
    }
    /* Make sure end of word is not lagging behind the cursor */
    comp_ctx->wordEnd = comp_ctx->cursor;
    /* If we're at the end of input, tell the caller about it */
    if(comp_ctx->cursor == end) {
        return stat_EOF;  /* Done: End of input */
    }
    return stat_OK;       /* Success! Skipped some whitespace! */
}

/* Find ending index of the current word (not for use on quoted strings) */
static comp_stat
lex_locate_end_of_word(comp_context_t *comp_ctx) {
    _assert_valid_comp_context();
    /* To allow this function to be called more than once while parsing  */
    /* the same word, only proceed if wordEnd <= cursor                  */
    if(comp_ctx->wordEnd > comp_ctx->cursor) {
        return stat_OK;  /* End of current word has already been located */
    }
    /* Compute the range of ctx->buf indexes to search within */
    u32 end = comp_ctx->len - 1;
    u32 start = comp_ctx->cursor;
    /* Scan forward from the cursor to find the end of the current word */
    u32 i;
    for(i = start; i <= end; i++) {
        switch(comp_ctx->buf[i]) {
            /* Stop for whitespace */
            case 0:    /* End of string may have null byte */
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                /* Upon seeing the first whitespace char, update rightmost */
                /* index to point at the previous byte                     */
                comp_ctx->wordEnd = (i > 0) ? (i - 1) : 0;
                return stat_OK;
            default:
                /* Input string may not end with whitespace */
                comp_ctx->wordEnd = i;
        }
    }
    /* This means end of word is also end of input */
    comp_ctx->wordEnd = end;
    return stat_OK;
}

/* Skip characters until delimiter, updating the line tracker */
static comp_stat
lex_skip_until(comp_context_t * comp_ctx, u8 delimiter) {
    _assert_valid_comp_context();
    /* Compute the range of ctx->buf indexes to search within */
    u32 end_index = comp_ctx->len - 1;
    u32 start_index = comp_ctx->cursor;
    /* Skip bytes until delimiter */
    u32 i;
    for(i = start_index; i <= end_index; i++) {
        u8 c = comp_ctx->buf[i];
        /* Delimiter might be '\n', but still need to update line tracker */
        if(c == '\n' && (i < end_index)) {
            comp_ctx->lineNum += 1;
            comp_ctx->lineStart = i + 1;
        }
        /* Stop if this byte is the delimiter */
        if(c == delimiter) {
            comp_ctx->cursor = (i + 1 <= end_index) ? i + 1 : end_index;
            _sync_wordEnd_to_cursor();
            return stat_OK;
        }
    }
    _sync_wordEnd_to_cursor();
    return stat_EOF;
}

/* Consume a comment bounded by () */
static comp_stat
lex_consume_paren_comment(comp_context_t * comp_ctx) {
    return lex_skip_until(comp_ctx, ')');
}

/* Consume a comment from # to end of line  */
static comp_stat
lex_consume_sharp_comment(comp_context_t * comp_ctx) {
    return lex_skip_until(comp_ctx, '\n');
}


/* ============== */
/* == Compiler == */
/* ============== */

/* Compile an integer literal as U8, U16, or I32 */
static comp_stat
compile_int_literal(mk_context_t * ctx, i32 n) {
    if(n >= 0) {
        if(n <= 255) {
            _assert_dictionary_free_space(2);
            _append_dictionary_byte(MK_U8);
            _append_dictionary_byte((u8)n);
            return stat_OK;
        }
        if(n <= 65535) {
            _assert_dictionary_free_space(3);
            _append_dictionary_byte(MK_U16);
            _append_dictionary_byte((u8)n);
            _append_dictionary_byte((u8)(n>>8));
            return stat_OK;
        }
    }
    _assert_dictionary_free_space(5);
    _append_dictionary_byte(MK_I32);
    _append_dictionary_byte((u8)n);
    _append_dictionary_byte((u8)(n>>8));
    _append_dictionary_byte((u8)(n>>16));
    _append_dictionary_byte((u8)(n>>24));
    return stat_OK;
}

/* Compile a double-quoted string literal */
static comp_stat
compile_str(comp_context_t *comp_ctx, mk_context_t * ctx) {

    /* ==================== */
    /* TODO: IMPLEMENT THIS */
    /* ==================== */

    return stat_StrSyntax;
}

/* Compile a single-quoted ASCII character literal */
static comp_stat
compile_char(comp_context_t *comp_ctx, mk_context_t * ctx) {
    comp_stat status = lex_locate_end_of_word(comp_ctx);
    if(status != stat_OK) {
        return status;
    }
    /* Calculate word's start position and length within the input buffer */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    /* Advance the cursor */
    if(comp_ctx->wordEnd + 1 < comp_ctx->len) {
        comp_ctx->wordEnd += 1;
        comp_ctx->cursor = comp_ctx->wordEnd;
    } else {
        comp_ctx->cursor = comp_ctx->wordEnd;
    }
    /* Try to compile as a U8 literal */
    if(length == 3) {
        /* Might be normal character like 'a', 'A', '0', etc. */
        if(buf[0] == '\'' && buf[2] == '\'') {
            _assert_dictionary_free_space(2);
            _append_dictionary_byte(MK_U8);
            _append_dictionary_byte(buf[1]);
            return stat_OK;
        }
    } else if(length == 4) {
        /* Might be '\n', '\t', '\r', or '\\' */
        if(buf[0] == '\'' && buf[1] == '\\' && buf[3] == '\'') {
            u8 esc;
            switch(buf[2]) {
                case 'n':
                    esc = '\n';
                    break;
                case 'r':
                    esc = '\r';
                    break;
                case 't':
                    esc = '\t';
                    break;
                case '\\':
                    esc = '\\';
                    break;
                default:
                    return stat_ParserError;
            }
            _assert_dictionary_free_space(2);
            _append_dictionary_byte(MK_U8);
            _append_dictionary_byte(esc);
            return stat_OK;
        }
    }
    return stat_ParserError;
}


/* ============ */
/* == Parser == */
/* ============ */

/* Parse a base-16 "0x"-prefixed hex integer literal. */
static comp_stat
parse_int_hex(comp_context_t *comp_ctx, mk_context_t * ctx) {
    /* Calculate word's start position and length within the input buffer */
    /* CAUTION! This expects lex_locate_end_of_word() was done by caller  */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    if(length > 2 && buf[0] == '0' && buf[1] == 'x') {
        _advance_cursor(2);
        u32 prev = 0;
        u32 curr = prev;
        int i;
        for(i = 2; i < length; i++) {
            curr *= 16;
            if('0' <= buf[i] && buf[i] <= '9') {
                curr += (buf[i] - '0');
            } else if('a' <= buf[i] && buf[i] <= 'f') {
                curr += (buf[i] - 'a' + 10);
            } else if('A' <= buf[i] && buf[i] <= 'F') {
                curr += (buf[i] - 'A' + 10);
            } else {
                return stat_IntSyntax;
            }
            /* Check for overflow */
            if(curr < prev) {
                return stat_IntOutOfRange;
            }
            prev = curr;
            _advance_cursor(1);
        }
        _sync_wordEnd_to_cursor();
        return compile_int_literal(ctx, (i32)curr);
    } else {
        return stat_IntSyntax;
    }
}

/* Parse a base-10 decimal integer literal. */
static comp_stat
parse_int_decimal(comp_context_t *comp_ctx, mk_context_t * ctx) {
    /* Calculate word's start position and length within the input buffer */
    /* CAUTION! This expects lex_locate_end_of_word() was done by caller  */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    if(length > 0) {
        i32 prev = 0;
        i32 curr = prev;
        int i;
        for(i = 0; i < length; i++) {
            curr *= 10;
            if('0' <= buf[i] && buf[i] <= '9') {
                curr += (buf[i] - '0');
            } else {
                return stat_IntSyntax;
            }
            /* Check for overflow */
            if(curr < prev) {
                return stat_IntOutOfRange;
            }
            prev = curr;
            _advance_cursor(1);
        }
        _sync_wordEnd_to_cursor();
        return compile_int_literal(ctx, curr);
    } else {
        return stat_IntSyntax;
    }
}

/* Parse a negative (leading hyphen) base-10 decimal integer literal. */
static comp_stat
parse_int_decimal_neg(comp_context_t *comp_ctx, mk_context_t * ctx) {
    /* Calculate word's start position and length within the input buffer */
    /* CAUTION! This expects lex_locate_end_of_word() was done by caller  */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    if(length > 1 && buf[0] == '-') {
        _advance_cursor(1);
        i32 prev = 0;
        i32 curr = prev;
        int i;
        for(i = 1; i < length; i++) {
            curr *= 10;
            if('0' <= buf[i] && buf[i] <= '9') {
                curr -= (buf[i] - '0');
            } else {
                return stat_IntSyntax;
            }
            /* Check for overflow */
            if(curr > prev) {
                return stat_IntOutOfRange;
            }
            prev = curr;
            _advance_cursor(1);
        }
        _sync_wordEnd_to_cursor();
        return compile_int_literal(ctx, curr);
    } else {
        return stat_IntSyntax;
    }
}

/* Parse an integer literal (detect hex or decimal and dispatch as needed) */
static comp_stat
parse_int(comp_context_t *comp_ctx, mk_context_t * ctx) {
    comp_stat status = lex_locate_end_of_word(comp_ctx);
    if(status != stat_OK) {
        return status;
    }
    /* Calculate word's start position and length within the input buffer */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    /* Detect hex or decimal */
    if(length > 2 && buf[0] == '0' && buf[1] == 'x') {
        return parse_int_hex(comp_ctx, ctx);
    } else {
        return parse_int_decimal(comp_ctx, ctx);
    }
}

static comp_stat
parse_dictionary_word(comp_context_t * comp_ctx, mk_context_t * ctx) {
    u16 hashmap_bin_offset = 0;
    int status = hash_name(comp_ctx, &hashmap_bin_offset);
    if(status != stat_OK) {
        return status;
    }
    /* TODO: Parse the word */

    return stat_EOF;
}

/* Parse words that are not int, char, or string literals. */
static comp_stat
parse_word(comp_context_t * comp_ctx, mk_context_t * ctx) {
    _assert_valid_comp_context();
    /* Scan ahead to locate the end index of this token */
    comp_stat status = lex_locate_end_of_word(comp_ctx);
    if(status != stat_OK) {
        return status;
    }
    /* Calculate word's start position and length within the input buffer */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    /* Try to match and compile the token */
    _assert_dictionary_free_space(10);
    switch(length) {
    case 1:
        switch(buf[0]) {
        case '@':
            _append_dictionary_byte(MK_LB);   /* @ */
            break;
        case '!':
            _append_dictionary_byte(MK_SB);   /* ! */
            break;
        case '+':
            _append_dictionary_byte(MK_ADD);  /* + */
            break;
        case '-':
            _append_dictionary_byte(MK_SUB);  /* - */
            break;
        case '*':
            _append_dictionary_byte(MK_MUL);  /* * */
            break;
        case '/':
            _append_dictionary_byte(MK_DIV);  /* / */
            break;
        case '%':
            _append_dictionary_byte(MK_MOD);  /* % */
            break;
        case '~':
            _append_dictionary_byte(MK_INV);  /* ~ */
            break;
        case '^':
            _append_dictionary_byte(MK_XOR);  /* ^ */
            break;
        case '|':
            _append_dictionary_byte(MK_OR);   /* | */
            break;
        case '&':
            _append_dictionary_byte(MK_AND);  /* & */
            break;
        case '>':
            _append_dictionary_byte(MK_GT);   /* > */
            break;
        case '<':
            _append_dictionary_byte(MK_LT);   /* < */
            break;
        case 'r':
            _append_dictionary_byte(MK_R);    /* r */
            break;
        case '.':
            _append_dictionary_byte(MK_DOT);  /* . */
            break;
        default:
            return parse_dictionary_word(comp_ctx, ctx);
        }
        break;
    case 2:
        switch((buf[0] << 8) | buf[1]) {
        case ('h' << 8) | '@':                /* h@ */
            _append_dictionary_byte(MK_LH);
            break;
        case ('h' << 8) | '!':                /* h! */
            _append_dictionary_byte(MK_SH);
            break;
        case ('w' << 8) | '@':                /* w@ */
            _append_dictionary_byte(MK_LW);
            break;
        case ('w' << 8) | '!':                /* w! */
            _append_dictionary_byte(MK_SW);
            break;
        case ('+' << 8) | '+':                /* 1+ */
            _append_dictionary_byte(MK_INC);
            break;
        case ('-' << 8) | '-':                /* 1- */
            _append_dictionary_byte(MK_DEC);
            break;
        case ('<' << 8) | '<':                /* << */
            _append_dictionary_byte(MK_SLL);
            break;
        case ('>' << 8) | '>':                /* >> */
            _append_dictionary_byte(MK_SRL);
            break;
        case ('>' << 8) | '=':                /* >= */
            _append_dictionary_byte(MK_GTE);
            break;
        case ('<' << 8) | '=':                /* <= */
            _append_dictionary_byte(MK_LTE);
            break;
        case ('=' << 8) | '=':                /* == */
            _append_dictionary_byte(MK_EQ);
            break;
        case ('!' << 8) | '=':                /* != */
            _append_dictionary_byte(MK_NE);
            break;
        case ('>' << 8) | 'r':                /* >r */
            _append_dictionary_byte(MK_MTR);
            break;
        case ('c' << 8) | 'r':                /* cr */
            _append_dictionary_byte(MK_CR);
            break;
        case ('.' << 8) | 'h':                /* .h */
            _append_dictionary_byte(MK_DOTH);
            break;
        case ('.' << 8) | 'S':                /* .S */
            _append_dictionary_byte(MK_DOTS);
            break;
        default:
            return parse_dictionary_word(comp_ctx, ctx);
        }
        break;
    case 3:
        switch((buf[0] << 16) | (buf[1] << 8) | buf[2]) {
        case ('n' << 16) | ('o' << 8) | 'p':   /* nop */
            _append_dictionary_byte(MK_NOP);
            break;
        case ('n' << 16) | ('e' << 8) | 'g':   /* neg */
            _append_dictionary_byte(MK_NEG);
            break;
        case ('>' << 16) | ('>' << 8) | '>':   /* >>> */
            _append_dictionary_byte(MK_SRA);
            break;
        case ('d' << 16) | ('u' << 8) | 'p':   /* dup */
            _append_dictionary_byte(MK_DUP);
            break;
        case ('.' << 16) | ('S' << 8) | 'h':   /* .Sh */
            _append_dictionary_byte(MK_DOTSH);
            break;
        case ('.' << 16) | ('R' << 8) | 'h':   /* .Rh */
            _append_dictionary_byte(MK_DOTRH);
            break;
        default:
            return parse_dictionary_word(comp_ctx, ctx);
        }
        break;
    case 4:
        switch((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]) {
        case ('h' << 24) | ('a' << 16) | ('l' << 8) | 't':  /* halt */
            _append_dictionary_byte(MK_HALT);
            break;
        case ('c' << 24) | ('a' << 16) | ('l' << 8) | 'l':  /* call */
            _append_dictionary_byte(MK_CALL);
            break;
        case ('d' << 24) | ('r' << 16) | ('o' << 8) | 'p':  /* drop */
            _append_dictionary_byte(MK_DROP);
            break;
        case ('o' << 24) | ('v' << 16) | ('e' << 8) | 'r':  /* over */
            _append_dictionary_byte(MK_OVER);
            break;
        case ('s' << 24) | ('w' << 16) | ('a' << 8) | 'p':  /* swap */
            _append_dictionary_byte(MK_SWAP);
            break;
        case ('e' << 24) | ('m' << 16) | ('i' << 8) | 't':  /* emit */
            _append_dictionary_byte(MK_EMIT);
            break;
        case ('d' << 24) | ('u' << 16) | ('m' << 8) | 'p':  /* dump */
            _append_dictionary_byte(MK_DUMP);
            break;
        default:
            return parse_dictionary_word(comp_ctx, ctx);
        }
        break;
    case 5:
        if(strncmp((const char *)buf, "rdrop", length) == 0) {
            _append_dictionary_byte(MK_RDROP);
            break;
        }
        if(strncmp((const char *)buf, "print", length) == 0) {
            _append_dictionary_byte(MK_PRINT);
            break;
        }
        return parse_dictionary_word(comp_ctx, ctx);
    default:
        return parse_dictionary_word(comp_ctx, ctx);
    }
    /* Advance the cursor */
    if(comp_ctx->wordEnd + 1 < comp_ctx->len) {
        comp_ctx->wordEnd += 1;
        comp_ctx->cursor = comp_ctx->wordEnd;
        return stat_OK;
    } else {
        comp_ctx->cursor = comp_ctx->wordEnd;
        return stat_EOF;
    }
}

/* Parse words that begin with a hyphen. */
static comp_stat
parse_leading_hyphen(comp_context_t * comp_ctx, mk_context_t * ctx) {
    _assert_valid_comp_context();
    comp_stat status = lex_locate_end_of_word(comp_ctx);
    if(status != stat_OK) {
        return status;
    }
    /* Calculate word's start position and length within the input buffer */
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);
    /* Check if second character is a digit */
    if((length > 1) && ('0' <= buf[1]) && (buf[1] <= '9')) {
        return parse_int_decimal_neg(comp_ctx, ctx);
    }
    return parse_word(comp_ctx, ctx);
}

/* Parse a lexical token, and invoke compiler functions as needed */
static comp_stat
parse_token(comp_context_t * comp_ctx, mk_context_t * ctx) {
    _assert_valid_comp_context();
    /* Check the first character of the next token */
    u8 c = comp_ctx->buf[comp_ctx->cursor];
    switch(c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return parse_int(comp_ctx, ctx);
        case '-':
            return parse_leading_hyphen(comp_ctx, ctx);
        case '"':
            return compile_str(comp_ctx, ctx);
        case '\'':
            return compile_char(comp_ctx, ctx);
        case '(':
            return lex_consume_paren_comment(comp_ctx);
        case '#':
            return lex_consume_sharp_comment(comp_ctx);
        default:
            return parse_word(comp_ctx, ctx);
    }
}


/* =================================== */
/* == Main Entry Point for Compiler == */
/* =================================== */

/* Compile Markab Script source from text into bytecode in ctx.RAM.       */
/* Compile error details get logged using mk_host_*() Host API functions. */
/* Returns: 1 = Success, 0 = Error (details get logged to Host API)       */
int
comp_compile_src(mk_context_t *ctx, const u8 * text, u32 text_len) {
    /* Initialize the compiler context for beginning of the first line */
    comp_context_t comp_ctx = {
        text,      /* .buf         */
        text_len,  /* .len         */
        1,         /* .line_number */
        0,         /* .line_start  */
        0,         /* .word_left   */
        0,         /* .word_right  */
    };
    /* Loop for long enough to process all the characters of the input text */
    /* Note that one iteration of the loop will typically consume multiple  */
    /* characters of input text, so usually the loop ends with a break.     */
    u32 i;
    comp_stat status = stat_OK;
    for(i = 0; i < text_len; i++) {
        /* Skip whitespace to find the start of next lexical token */
        status = lex_skip_whitespace(&comp_ctx);
        if(status != stat_OK) {
            break;
        }
        status = parse_token(&comp_ctx, ctx);
        if(status != stat_OK) {
            break;
        }
    }
    switch(status) {
        case stat_OK:   /* Odd, but OK I guess? 0-length input? */
            return 1;
        case stat_EOF:  /* Normal exit path                     */
            return 1;
        default:        /* Some type of error */
            log_compiler_error(&comp_ctx, status);
            return 0;
    }
}

#endif /* LIBMKB_COMP_C */
