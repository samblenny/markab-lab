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
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "libmarkab.h"
#include "markab_host_api.h"

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
    uint32_t cursorRow;
    uint32_t cursorCol;
    uint32_t maxRow;
    uint32_t maxCol;
} ttyState_t;
ttyState_t TTY;

typedef enum {
    CursorPosCurrent,
    CursorPosMaxPossible1,
    CursorPosMaxPossible2,
} cursorRequest_t;

cursorRequest_t CURSOR_REQUEST;

void restore_old_terminal_config() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_config);
}

// Restore old terminal config if process gets killed by a segfault, illegal
// instruction, or whatever. Without this handler, sudden exits will leave the
// terminal badly configured such that using the shell normally again will
// require running `reset`.
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

stack_t alt_stack;
char stack_buf[SIGSTKSZ];

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
    memset(&a, 0, sizeof(sigaction));
    a.sa_handler = catch_signal;
    sigfillset(&a.sa_mask);
    a.sa_flags = SA_ONSTACK;
    sigaction(SIGILL, &a, NULL);   // Hook for illegal instruction
    sigaction(SIGSEGV, &a, NULL);  // Hook for segmentation fault
    // Configure terminal stdio for unbuffered raw input
    tcflag_t imask = ~(IXON|IXOFF|ISTRIP|INLCR|IGNCR|ICRNL);
    tcflag_t Lmask = ~(ICANON|ECHO|ECHONL|ISIG|IEXTEN);
    struct termios new_config = old_config;
    new_config.c_iflag &= imask;
    new_config.c_lflag &= Lmask;
    new_config.c_cflag |= CS8;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_config);
    // Configure terminal stdout for unbuffered output
    setbuf(stdout, NULL);
}

// Reset all state
void cold_boot() {
    for(int i=0; i<BUF_SIZE; i++) {
        TIB[i] = 0;
        ISB[i] = 0;
    }
    TIB_LEN = 0;
    ISB_LEN = 0;
    TTY.state = Normal;
    TTY.csParam1 = 1;
    TTY.csParam2 = 1;
    TTY.cursorRow = 0;
    TTY.cursorCol = 0;
    TTY.maxRow = 0;
    TTY.maxCol = 0;
    CURSOR_REQUEST = CursorPosCurrent;
}

void tib_backspace() {
    // Delete one character
    if(TIB_LEN > 0) {
        TIB[TIB_LEN] = 0;
        TIB_LEN--;
    }
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

void tib_cr() {
    // TODO: Send the composed line somewhere to be interpreted
    printf(" CR\n");
    TIB_LEN = 0;
    TIB[TIB_LEN] = 0;
}

void tty_cursor_goto(uint32_t row, uint32_t col) {
    printf("\33[%u;%uH", row, col);
}

void tty_cursor_request_position(cursorRequest_t crq) {
    // This may trigger a chain of stuff in the state machine
    CURSOR_REQUEST = crq;
    printf("\33[6n");
}

void tty_cursor_handle_report(uint32_t row, uint32_t col) {
    switch(CURSOR_REQUEST) {
    case CursorPosCurrent:
        TTY.cursorRow = row;
        TTY.cursorCol = col;
        TTY.state = Normal;
        return;
    case CursorPosMaxPossible1:
        // Save original cursor position
        TTY.cursorRow = row;
        TTY.cursorCol = col;
        // Attempt to move to an offscreen cursor position that should clip
        // the row and col to their maximum possible values
        tty_cursor_goto(999, 999);
        // Chain another request to get the clipped values and put the
        // cursor back in its original position
        tty_cursor_request_position(CursorPosMaxPossible2);
        TTY.state = Normal;
        return;
    case CursorPosMaxPossible2:
        // Save the maximum row and column
        TTY.maxRow = row;
        TTY.maxCol = col;
        // Restore original cursor position. This needs CursorPosMaxPossible1
        // to have already saved the original cursor position.
        tty_cursor_goto(TTY.cursorRow, TTY.cursorCol);
        // End the request
        CURSOR_REQUEST = CursorPosCurrent;
        TTY.state = Normal;
        return;
    }
}

void tty_update_line() {
    // Erase all of current line, move to column 1
    char *erase_line = "\33[2K\33[G";
    write(STDOUT_FILENO, erase_line, 7);
    // If the input buffer is too long, show only the end
    if(TTY.maxCol < TIB_LEN) {
        size_t diff = TIB_LEN - TTY.maxCol;
        write(STDOUT_FILENO, TIB+diff, TTY.maxCol);
        return;
    }
    // Otherwise, show the whole input buffer
    write(STDOUT_FILENO, TIB, TIB_LEN);
}

void step_tty_state(unsigned char c) {
    switch(TTY.state) {
    case Normal:
        switch(c) {
        case 13:  // Enter key is CR (^M) since ICRNL|INLCR are turned off
            tib_cr();
            return;
        case 27:
            TTY.state = Esc;
            return;
        case 127:
            tib_backspace();
            tty_update_line();
            return;
        case 'L'-64:
            // For Control-L, check window size
            tty_cursor_request_position(CursorPosMaxPossible1);
        default:
            if(c < ' ' || c > 0xf7) {
                // Ignore control characters and invalid UTF-8 bytes
                return;
            }
            // Handle normal character or UTF-8 byte
            tib_insert(c);
            tty_update_line();
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
            tty_cursor_handle_report(TTY.csParam1, TTY.csParam2);
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

int main() {
    set_terminal_for_raw_unbuffered_input();
    cold_boot();
    markab_init();
    tty_cursor_request_position(CursorPosMaxPossible1);
    unsigned char control_c = 'C' - 64;
    for(;;) {
        // Block until input is available (or EOF)
        ISB_LEN = read(STDIN_FILENO, ISB, BUF_SIZE);
        if(ISB_LEN == 0) {
            goto done;  // end for EOF
        }
        // Dump input chars
        int debug_enable = 0;
        if(debug_enable) {
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
                } else if(c > 0xf7) {
                    printf(" %u", c);
                } else {
                    printf(" %c", c);
                }
            }
            printf(" ===\n");
        }
        // Feed all the input chars to the tty state machine
        for(int i=0; i<ISB_LEN; i++) {
            unsigned char c = ISB[i];
            if(c == control_c) {
                goto done;  // end for ^C
            }
            step_tty_state(c);
        }
    }
done:
    printf("\n");
    return 0;
    // There should be an atexit() hook happening here to restore the terminal
    // configuration.
}
