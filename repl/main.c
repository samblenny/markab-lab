#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "libmarkab.h"

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
    EscCtrlSeq,     // Control sequence; CS Intro (CSI) is `Esc [`
} ttyInputState_t;

// Terminal State Machine Variables
typedef struct {
    ttyInputState_t state;
} ttyState_t;
ttyState_t TTY;

void restore_old_terminal_config() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_config);
}

void set_terminal_for_raw_unbuffered_input() {
    // Set hook to restore original terminal attributes during exit
    tcgetattr(STDIN_FILENO, &old_config);
    atexit(restore_old_terminal_config);
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

void tty_update_line() {
    char *erase_line = "\33[2K\33[G";  // erase whole line, move to column 1
    write(STDOUT_FILENO, erase_line, 7);
    write(STDOUT_FILENO, TIB, TIB_LEN);
}

void step_tty_state(unsigned char c) {
    switch(TTY.state) {
    case Normal:
        switch(c) {
        case 13:  // Enter key is CR (^M) since ICRNL|INLCR are turned off
            tib_cr();
            break;
        case 27:
            TTY.state = Esc;
            break;
        case 127:
            tib_backspace();
            tty_update_line();
            break;
        default:
            if(c < ' ') {
                // Ignore control characters
            } else if(c > 0xf7) {
                // Ignore invalid UTF-8 characters
            } else {
                // Handle normal character or UTF-8 byte
                tib_insert(c);
                tty_update_line();
            }
        }
        break;
    case Esc:
        switch(c) {
        case 'N':
        case 'O':
            TTY.state = EscSingleShift;
            break;
        case '[':
            TTY.state = EscCtrlSeq;
            break;
        case 27:
            TTY.state = EscEsc;
            break;
        default:
            // Switch back to normal mode if the escape sequence was not one of
            // the mode-shifting sequences that we know about
            TTY.state = Normal;
        }
        break;
    case EscEsc:
        // For now, filter Meta-<whatever> sequences out of input stream. This
        // catches stuff like Meta-Up (`Esc Esc [ A`).
        break;
    case EscSingleShift:
        // The SS2 (G2) single shift charset is boring. The SS3 (G3) charset
        // has F1..F4. But, for now, just filter out all of that stuff.
        TTY.state = Normal;
        break;
    case EscCtrlSeq:
        if((c >= '0' && c <= '9') || c == ';') {
            // Ignore numeric parameters (numbers and ';'). Parsing numeric
            // parameters is necessary for F-keys, Ins, Del, PgDn, PgUp, cursor
            // position reports, etc. But, for now, just filter that stuff out.
        } else {
            // This is the spot to parse and handle arrow keys:
            //    'A': up, 'B': down, 'C': right, 'D': left
            // But, for now, just filter them out.
            TTY.state = Normal;
        }
        break;
    }
}

int main() {
    set_terminal_for_raw_unbuffered_input();
    cold_boot();
    markab_init();
    unsigned char control_c = 'C' - 64;
    for(;;) {
        // Block until input is available (or EOF)
        ISB_LEN = read(STDIN_FILENO, ISB, BUF_SIZE);
        if(ISB_LEN == 0) {
            goto done;  // end for EOF
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
