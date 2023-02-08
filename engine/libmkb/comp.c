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

/* Lexical token types */
typedef enum {
    lexT_StringLit,      /* Double-quote string literal          */
    lexT_CharLit,        /* Single-quote character literal       */
    lexT_ParenComment,   /* (...) style parentheses comment      */
    lexT_SharpComment,   /* #... style sharp-sign comment        */
    lexT_Word,           /* Word name, or maybe a number literal */
} lex_token_t;

/* Compiler error status codes */
typedef enum {
    stat_OK,          /* Success                              */
    stat_BadContext,  /* Compiler context struct is corrupted */
    stat_EOF,         /* End of input                         */
} comp_stat;


/* ============ */
/* == Macros == */
/* ============ */

/* Macro expression to compute if word boundaries are within buffer length */
/* This evaluates to 1 if the context is valid or 0 if context is invalid. */
#define _context_is_valid(CTX)  (        \
    ((CTX)->cursor <= (CTX)->wordEnd) |  \
    ((CTX)->wordEnd  < (CTX)->len)       )

/* Macro to compute pointer to start of word from compiler context */
#define _word_pointer(CTX) (&(CTX)->buf[(CTX)->cursor])

/* Macro to compute length of word from compiler context */
#define _word_length(CTX) ((CTX)->wordEnd - (CTX)->cursor + 1)


/* ======================= */
/* == Utility Functions == */
/* ======================= */

/* Hash a name string into a bin offset using multiply-with-carry (MWC) hash */
static comp_stat
hash_name(comp_context_t * comp_ctx, u16 * hash_bin_offset) {
    /* Calculate name's start position and length within the input buffer */
    if(!_context_is_valid(comp_ctx)) {
        return stat_BadContext;
    }
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
skip_whitespace(comp_context_t * comp_ctx) {
    /* First make sure the compiler context hasn't gotten corrupted */
    if(!_context_is_valid(comp_ctx)) {
        return stat_BadContext;
    }
    /* Compute the range of ctx->buf indexes to search within */
    u32 end = comp_ctx->len - 1;
    u32 start = comp_ctx->cursor;
    /* Skip whitespace! Note, this may advance the cursor too far. */
    u32 i;
    u8 done = 0;
    for(i = start; i <= end && !done; i++) {
        switch(comp_ctx->buf[i]) {
            /* For space, tab, or a CR that is probably the start of a  */
            /* CR/LF Windows-style line ending, jsut advance the cursor */
            case ' ':
            case '\t':
            case '\r':
                comp_ctx->cursor = i + 1;
                break;
            /* For LF newlines, advance cursor and update the line tracker */
            case '\n':
                comp_ctx->cursor = i + 1;
                comp_ctx->lineStart = i + 1;
                comp_ctx->lineNum += 1;
                break;
            default:
                done = 1;
        }
    }
    /* If the loop advanced the cursor out of range, move it back to the end */
    /* of the buffer to avoid leaving the context struct in an invalid state */
    int end_of_input = 0;
    if(comp_ctx->cursor >= comp_ctx->len) {
        comp_ctx->cursor = comp_ctx->len - 1;
        end_of_input = 1;
    }
    comp_ctx->wordEnd = comp_ctx->cursor;
    /* If we're at the end of input, tell the caller about it */
    if(end_of_input) {
        return stat_EOF;  /* Done: End of input */
    }
    return stat_OK;       /* Success! Skipped some whitespace! */
}

/* Find the start and end indexes of the next word. */
static comp_stat
seek_word(comp_context_t *comp_ctx) {
    /* Make sure the compiler context hasn't gotten corrupted */
    if(!_context_is_valid(comp_ctx)) {
        return stat_BadContext;
    }
    /* Compute the range of ctx->buf indexes to search within */
    u32 end = comp_ctx->len - 1;
    u32 start = comp_ctx->cursor;
    /* Scan forward from the cursor to find the end of the current word */
    u32 i;
    u32 new_rightmost_index = comp_ctx->wordEnd;
    u8 done = 0;
    for(i = start; i <= end && !done; i++) {
        switch(comp_ctx->buf[i]) {
            /* Stop for whitespace */
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                done = 1;
            /* For non-whitespace, update the rightmost index */
            default:
                new_rightmost_index += 1;
        }
    }
    comp_ctx->wordEnd = new_rightmost_index;  /* Commit the change */
    return stat_OK;                           /* Success! Found the word end */
}

/* Set token_type to next lexical token's type: quoted string, comment, etc. */
static comp_stat
lex_detect_type(comp_context_t * comp_ctx, lex_token_t * token_type) {
    /* First make sure the compiler context hasn't gotten corrupted */
    if(!_context_is_valid(comp_ctx)) {
        return stat_BadContext;
    }
    /* Check the first character of the next token */
    u8 c = comp_ctx->buf[comp_ctx->cursor];
    switch(c) {
        case '"':
            *token_type = lexT_StringLit;
            return stat_OK;
        case '\'':
            *token_type = lexT_CharLit;
            return stat_OK;
        case '(':
            *token_type = lexT_ParenComment;
            return stat_OK;
        case '#':
            *token_type = lexT_SharpComment;
            return stat_OK;
        default:
            *token_type = lexT_Word;
            return stat_OK;
    }
}

/* Skip characters until delimiter, updating the line tracker. */
static comp_stat
lex_skip_until(comp_context_t * comp_ctx, u8 delimiter) {
    /* First make sure the compiler context hasn't gotten corrupted */
    if(!_context_is_valid(comp_ctx)) {
        return stat_BadContext;
    }
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
            return stat_OK;
        }
    }
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


/* ======================= */
/* == Parser + Compiler == */
/* ======================= */

/* Parse and compile the current word, possibly consuming additional words. */
static comp_stat
compile_word(comp_context_t * comp_ctx, mk_context_t * ctx) {
    /* Calculate word's start position and length within the input buffer */
    if(!_context_is_valid(comp_ctx)) {
        return stat_BadContext;
    }
    const u8 * buf = _word_pointer(comp_ctx);
    u32 length = _word_length(comp_ctx);

    u16 hashmap_bin_offset = 0;
    int status = hash_name(comp_ctx, &hashmap_bin_offset);
    if(status != stat_OK) {
        return status;
    }
    /* TODO: Parse the word */

    return stat_EOF;
}

/* Compile a double-quoted string literal */
static comp_stat
compile_str(comp_context_t *comp_ctx, mk_context_t * ctx) {
    return stat_EOF;
}

/* Compile a single-quoted ASCII character literal */
static comp_stat
compile_char(comp_context_t *comp_ctx, mk_context_t * ctx) {
    return stat_EOF;
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
        status = skip_whitespace(&comp_ctx);
        if(status != stat_OK) {
            break;              /* Stop looping */
        }
        /* Peek at next token's lexical type: quoted literal, comment, etc. */
        lex_token_t token_type = lexT_Word;
        status = lex_detect_type(&comp_ctx, &token_type);
        if(status != stat_OK) {
            break;               /* Stop looping */
        }
        /* Handle next token according to lexical type */
        comp_stat status;
        switch(token_type) {
            case lexT_StringLit:
                status = compile_str(&comp_ctx, ctx);
                break;
            case lexT_CharLit:
                status = compile_char(&comp_ctx, ctx);
                break;
            case lexT_ParenComment:
                status = lex_consume_paren_comment(&comp_ctx);
                break;
            case lexT_SharpComment:
                status = lex_consume_sharp_comment(&comp_ctx);
                break;
            case lexT_Word:
                /* Fall through to default */
            default:
                status = compile_word(&comp_ctx, ctx);
        }
        if(status != stat_OK) {
            break;               /* Stop looping */
        }
    }
    if(status == stat_BadContext) {
        return 0;  /* Error: compiler context got corrupted */
    }
    /* Success! */
    return 1;
}

#endif /* LIBMKB_COMP_C */
