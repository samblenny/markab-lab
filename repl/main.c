// Copyright (c) 2022 Sam Blenny
// SPDX-License-Identifier: MIT
//
// Markab Forth System REPL
//
// The `#define _XOPEN_SOURCE 500` is to enable XOPEN (kinda like POSIX?)
// features for signal handling, because I kept segfaulting and having to reset
// my terminal to get my shell prompt working again. For background, refer to:
// - `man signal sigaction sigaltstack`
// - /usr/include/features.h (on Debian 11)
// - https://pubs.opengroup.org/onlinepubs/007904875/basedefs/signal.h.html
//
#define _XOPEN_SOURCE 500
#include <errno.h>      // EINTR
#include <sys/ioctl.h>  // TIOCGWINSZ
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "libmarkab.h"
#include "markab_host_api.h"

//----------------
//#define DEBUG_EN
//----------------

// Struct to hold original terminal config to be restored during exit
struct termios old_config;

// Size for buffers
#define BUF_SIZE 1024

// Text Input Buffer: line of text currently being edited
unsigned char TIB[BUF_SIZE];
size_t TIB_LEN = 0;

// Input Stream Buffer: bytes read from stdin (includes escape sequences)
unsigned char ISB[BUF_SIZE];
size_t ISB_LEN;
int STDIN_ISATTY = 0;

// States of the terminal input state machine.
// CSI stands for Control Sequence Intro, and it means `Esc [`.
typedef enum {
    Normal,         // Ready for input, non-escaped
    Esc,            // Start of escape sequence
    EscEsc,         // Start of meta-combo escape sequence
    EscSingleShift, // Single shift: `Esc N <foo>`, `Esc O <foo>`, etc.
    EscCSIntro,     // Control sequence Intro, `Esc [`
    EscCSParam1,    // first control sequence parameter `[0-9]*;`
    EscCSParam2,    // second control sequence parameter `[0-9]*`
} ttyInputState_t;

// Terminal State Machine Variables
typedef struct {
    ttyInputState_t state;
    uint32_t csParam1;
    uint32_t csParam2;
    uint32_t maxRow;
    uint32_t maxCol;
} ttyState_t;
ttyState_t TTY;

// Alternate stack for SIGSEGV handler
stack_t alt_stack;
char stack_buf[SIGSTKSZ];

// Flags
int GOT_SIGWINCH = 0;

// Exit hook handler to restore original terminal configuration
void restore_old_terminal_config() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_config);
}

// Handle signals. Mostly this is to restore old terminal config if process
// gets killed by a segfault, illegal instruction, or whatever. Without this
// handler, sudden exits will leave the terminal badly configured such that
// using the shell normally again will require running `reset`.
void catch_signal(int signal) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_config);
    switch(signal) {
    case SIGSEGV:
        fprintf(stderr, "[Segmentation Fault (restoring terminal config)]\n");
        break;
    case SIGILL:
        fprintf(stderr, "[Illegal Instruction (restoring terminal config)]\n");
        break;
    default:
        // See list in /usr/include/x86_64-linux-gnu/bits/signum-generic.h
        fprintf(stderr, "[signal %u: restoring terminal config]\n", signal);
    }
    exit(1);
}

// Handle a window resize notification (quickly).
void handle_sigwinch(int signal) {
    // Doing stuff in this signal handler is risky due to potential concurrency
    // issues, so just set a flag and go. The tty state machine will take care
    // of detecting the new terminal size later.
    GOT_SIGWINCH = 1;
}

// Detect terminal window size in rows and columns
void update_terminal_size() {
    struct winsize ws;
    memset(&ws, 0, sizeof(ws));
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    TTY.maxRow = ws.ws_row;
    TTY.maxCol = ws.ws_col;
    GOT_SIGWINCH = 0;
}

// Set up an alternate stack for sigaction(SIGSEGV, ...) to use. Otherwise, if
// the main stack got messed up, which it most likely did (because segfault),
// trying to handle the first SIGSEGV will just segfault again before the
// handler accomplishes anything useful.
void init_alt_stack() {
    alt_stack.ss_sp = stack_buf;
    alt_stack.ss_size = SIGSTKSZ;
    alt_stack.ss_flags = 0;
    sigaltstack(&alt_stack, NULL);
}

// This needs `#define _XOPEN_SOURCE` to work right
void set_terminal_for_raw_unbuffered_input() {
    // Set hook to restore original terminal attributes during exit
    tcgetattr(STDIN_FILENO, &old_config);
    atexit(restore_old_terminal_config);  // Hook for normal exit
    init_alt_stack();
    struct sigaction a;
    memset(&a, 0, sizeof(a));
    a.sa_handler = catch_signal;
    sigfillset(&a.sa_mask);
    a.sa_flags = SA_ONSTACK;
    sigaction(SIGILL, &a, NULL);   // Hook for illegal instruction
    sigaction(SIGSEGV, &a, NULL);  // Hook for segmentation fault
    a.sa_handler = handle_sigwinch;
    sigaction(SIGWINCH, &a, NULL); // Hook for window resize notification
    // Configure terminal stdio for unbuffered raw input
    STDIN_ISATTY = isatty(STDIN_FILENO);
    tcflag_t imask = ~(IXON|IXOFF|ISTRIP|INLCR|IGNCR|ICRNL);
    tcflag_t Lmask = ~(ICANON|ECHO|ECHONL|ISIG|IEXTEN);
    struct termios new_config = old_config;
    new_config.c_iflag &= imask;
    new_config.c_lflag &= Lmask;
    new_config.c_cflag |= CS8;
    if(STDIN_ISATTY) {
        new_config.c_cc[VMIN] = 0;   // read() can return 0 if buffer is empty
        new_config.c_cc[VTIME] = 1;  // read() non-blocking timeout (unit 0.1s)
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &new_config);
    // Configure terminal stdout for unbuffered output
    setbuf(stdout, NULL);
}

// Reset all state
void reset_state() {
    for(int i=0; i<BUF_SIZE; i++) {
        TIB[i] = 0;
        ISB[i] = 0;
    }
    TIB_LEN = 0;
    ISB_LEN = 0;
    TTY.state = Normal;
    TTY.csParam1 = 1;
    TTY.csParam2 = 1;
    TTY.maxRow = 0;
    TTY.maxCol = 0;
}

// Peek at Unicode codepoint for UTF-8 seqence ending at TIB[index-1]
uint32_t tib_peek_prev_code(size_t current_index) {
    if(current_index == 0) {
        return 0;
    }
    uint32_t code = 0;
    uint32_t shift = 0;
    for(int i=current_index; i>0;) {
        i--;
        uint8_t c = TIB[i];
        if((c & 0xC0) == 0x80) {          // Continuation byte
            code |= (c & 0x3F) << shift;
            shift += 6;
            continue;
        }
        if((c < 128) || (c > 0xF7)) {     // ASCII or invalid UTF-8 byte
            code = c;
        } else if((c & 0xE0) == 0xC0) {   // Leading byte of 2-byte sequence
            code |= (c & 0x1F) << shift;
        } else if((c & 0xF0) == 0xE0) {   // Leading byte of 3-byte sequence
            code |= (c & 0x0F) << shift;
        } else if((c & 0xF8) == 0xF0) {   // Leading byte of 4-byte sequence
            code |= (c & 0x07) << shift;
        }
        break;
    }
    return code;
}

// Check if codepoint is Unicode Reginal Indicator Symbol Letter
int is_flag_letter(uint32_t code) {
    return (code >= 0x1F1E6) && (code <= 0x1F1FF);
}

// Check if codepoint is Unicode Emoji Modifier
int is_emoji_modifier(uint32_t code) {
    return (code >= 0x1F3FB) && (code <=0x1F3FF);
}

// Delete one UTF-8 grapheme cluster, which may be one or more UTF-8 codepoints
// consisting of 1, some, or many bytes. This is intended to work with plain
// ASCII, single-codepoint Unicode characters, and multi-codepoint Unicode
// grapheme clusters. In particular, it should hopefully handle modern emoji.
void tib_backspace(int max_depth) {
    if(TIB_LEN == 0 || max_depth < 1) {
        return;
    }
    uint32_t code = 0;
    uint32_t shift = 0;
    uint8_t c = 0;
    // Delete the last UTF-8 sequence (1..4 bytes) in the TIB
    for(; TIB_LEN>0;) {
        TIB_LEN--;
        c = TIB[TIB_LEN];
        TIB[TIB_LEN] = 0;
        if((c & 0xC0) == 0x80) {          // Continuation byte
            code |= (c & 0x3F) << shift;
            shift += 6;
            continue;
        }
        if((c < 128) || (c > 0xF7)) {     // ASCII or invalid UTF-8 byte
            code = c;
        } else if((c & 0xE0) == 0xC0) {   // Leading byte of 2-byte sequence
            code |= (c & 0x1F) << shift;
        } else if((c & 0xF0) == 0xE0) {   // Leading byte of 3-byte sequence
            code |= (c & 0x0F) << shift;
        } else if((c & 0xF8) == 0xF0) {   // Leading byte of 4-byte sequence
            code |= (c & 0x07) << shift;
        }
        break;
    }
    #ifdef DEBUG_EN
    printf(" delete: U+%04x\n", code);
    #endif

    // Check the value of the just-deleted codepoint, and the codepoint before
    // that one, to recursively finish delete the rest of the current grapheme
    // cluster. This is mainly for modern emoji.
    uint32_t prev = tib_peek_prev_code(TIB_LEN);
    switch(code) {
    case 0xFE0F:                          // Variation selector (for emoji)
    case 0x200D:                          // Zero width joiner
        tib_backspace(max_depth-1);
        return;
    default:
        // This uses special value of max_depth to prevent consuming more than
        // two regional indicator letter codepoints at a time, because flags
        // have two regional indicator letters. The point is to only delete the
        // last flag, if there happens to be a sequence of flags.
        if(is_flag_letter(code) && is_flag_letter(prev) && max_depth > 1) {
            tib_backspace(1);
            return;
        }
        if(is_emoji_modifier(code)) {
            tib_backspace(max_depth-1);
            return;
        }
        switch(prev) {
        case 0x200D:                      // Zero width joiner
            tib_backspace(max_depth-1);
            return;
        }
    }
}

// Clear screen and input buffer
void tib_clear_screen() {
    TIB_LEN = 0;                // clear input buffer
    TIB[TIB_LEN] = 0;
    printf("\33[H");            // cursor to top left
    printf("\33[2J");           // clear entire screen
    markab_outer(TIB, TIB_LEN); // trigger an OK prompt
}

void tib_cr() {
    markab_outer(TIB, TIB_LEN);
    TIB_LEN = 0;
    TIB[TIB_LEN] = 0;
}

void tib_insert(unsigned char c) {
    if(c == 0 || c == 127 || c > 0xf7 || (TIB_LEN+1 >= BUF_SIZE)) {
        // Silently ignore nulls, backspaces, and invalid UTF-8 bytes. Or, if
        // buffer is already full, ignore all characters.
        return;
    }
    // Otherwise, insert the character
    TIB[TIB_LEN] = c;
    TIB_LEN++;
    TIB[TIB_LEN] = 0;
}

void tty_update_line(int last_char_is_cr) {
    if(!STDIN_ISATTY) {
        // If STDIN is pipe or file, print the input buffer only for CR or EOF,
        // and do not use any ANSI escape sequences
        if(last_char_is_cr) {
            write(STDOUT_FILENO, TIB, TIB_LEN);
        }
    } else {
        // When STDIN is a TTY, for each new character, erase all of the
        // current line with an ANSI escape sequence and move back to column 1
        char *erase_line = "\33[2K\33[G";
        write(STDOUT_FILENO, erase_line, 7);
        // If the input buffer is too long, show only the tail of the buffer
        if(TTY.maxCol < TIB_LEN) {
            size_t diff = TIB_LEN - TTY.maxCol;
            write(STDOUT_FILENO, TIB+diff, TTY.maxCol);
            return;
        }
        // Otherwise, show the whole input buffer
        write(STDOUT_FILENO, TIB, TIB_LEN);
    }
}

void step_tty_state(unsigned char c) {
    switch(TTY.state) {
    case Normal:
        switch(c) {
        case 13:  // Enter key is CR (^M) since ICRNL|INLCR are turned off
            if(!STDIN_ISATTY) {
                tty_update_line(1);
            }
            tib_cr();
            return;
        case 12:  // Form feed (^L) clears screen
            tib_clear_screen();
            return;
        case 27:
            TTY.state = Esc;
            return;
        case 127:
            tib_backspace(TIB_LEN);
            tty_update_line(0);
            return;
        default:
            if(c < ' ' || c > 0xf7) {
                // Ignore control characters and invalid UTF-8 bytes
                return;
            }
            // Handle normal character or UTF-8 byte
            tib_insert(c);
            tty_update_line(0);
            return;
        }
    case Esc:
        switch(c) {
        case 'N':
        case 'O':
            TTY.state = EscSingleShift;
            return;
        case '[':
            TTY.state = EscCSIntro;
            return;
        case 27:
            TTY.state = EscEsc;
            return;
        default:
            // Switch back to normal mode if the escape sequence was not one of
            // the mode-shifting sequences that we know about
            TTY.state = Normal;
            return;
        }
    case EscEsc:
        switch(c) {
        case '[':
            // This is the third byte of a meta-key combo escape sequence
            return;
        default:
            // For now, filter Meta-<whatever> sequences out of input stream.
            // This catches stuff like Meta-Up (`Esc Esc [ A`).
            TTY.state = Normal;
            return;
        }
    case EscSingleShift:
        // The SS2 (G2) single shift charset is boring. The SS3 (G3) charset
        // has F1..F4. But, for now, just filter out all of that stuff.
        TTY.state = Normal;
        return;
    case EscCSIntro:
        TTY.csParam1 = 0;
        TTY.csParam2 = 0;
        if(c >= '0' && c <= '9') {
            TTY.csParam1 = c - '0';
            TTY.state = EscCSParam1;
            return;
        }
        switch(c) {
        case ';':
            TTY.state = EscCSParam2;
            return;
        default:
            // This is the spot to parse and handle arrow keys:
            //    'A': up, 'B': down, 'C': right, 'D': left
            // But, for now, just filter them out.
            TTY.state = Normal;
            return;
        }
    case EscCSParam1:
        if(c >= '0' && c <= '9') {
            TTY.csParam1 *= 10;
            TTY.csParam1 += c - '0';
            return;
        }
        switch(c) {
        case ';':
            TTY.state = EscCSParam2;
            return;
        default:
            // Getting a `~` here means F-key, Ins, Del, PgUp, PgDn, etc.
            // depending on the value of parameter 1
            TTY.state = Normal;
            return;
        };
    case EscCSParam2:
        if(c >= '0' && c <= '9') {
            TTY.csParam2 *= 10;
            TTY.csParam2 += c - '0';
            return;
        }
        switch(c) {
        case 'R':
            // Got cursor position report as answer to a "\33[6n" request
            // For now, just ignore this
            TTY.state = Normal;
            return;
        default:
            TTY.state = Normal;
            return;
        }
    }
}

size_t mkb_host_write(const void *buf, size_t count) {
    return write(STDOUT_FILENO, buf, count);
}

// Do a non-blocking read from stdin to update the tty state machine
// Return codes
//  -1 --> EOF or ^C
//   0 --> normal
int mkb_host_step_stdin() {
    // Handle pending tasks
    if(GOT_SIGWINCH) {
        update_terminal_size();
        GOT_SIGWINCH = 0;
    }
    // Check for input (read() is configured with a 0.1s timeout)
    int result = read(STDIN_FILENO, ISB, BUF_SIZE);
    if(result == 0 && !STDIN_ISATTY) {
        // In case of EOF when reading from file or pipe, insert CR, then stop
        unsigned char cr = 13;
        step_tty_state(cr);
        return -1;
    }
    if(result < 0) {
        return 0;  // Error (TODO: handle this better)
    }
    ISB_LEN = result;
#ifdef DEBUG_EN
    // Dump input chars
    printf("\n===");
    for(int i=0; i<ISB_LEN; i++) {
        unsigned char c = ISB[i];
        if(c == 27) {
            printf(" Esc");
        } else if(c == ' ') {
            printf(" Spc");
        } else if(c == 13) {
            printf(" Cr");
        } else if(c < ' ') {
            printf(" ^%c", c+64);
        } else if(c == ' ') {
            printf(" Spc");
        } else if(c == 127) {
            printf(" Back");
        } else if(c >= 128) {
            printf(" %02X", c);
        } else {
                printf(" %c", c);
        }
    }
    printf(" ===\n");
#endif
    // Feed all the input chars to the tty state machine
    for(int i=0; i<ISB_LEN; i++) {
        unsigned char c = ISB[i];
        if(c == 'C' - 64) {
            return -1;          // end for ^C
        }
        step_tty_state(c);
    }
    return 0;
}

int main() {
    set_terminal_for_raw_unbuffered_input();
    reset_state();
    // Detect terminal window size
    if(STDIN_ISATTY) {
        // Use ioctl TIOCGWINSZ method to detect current terminal window size
        update_terminal_size();
    }
    // Transfer control to Markab outer interpreter which is expected to
    // 1. Repeatedly call mkb_host_step_stdin()
    // 2. Not return until it is ready for the process to exit.
    markab_cold();
    return 0;
    // There should be an atexit() hook happening here to restore the terminal
    // configuration.
}
