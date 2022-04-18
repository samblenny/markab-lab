#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

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
    Normal,        // Ready for input, non-escaped
    Esc,           // Start of escape sequence
    EscSS2,        // Single shift of form `Esc N <foo>` (G2 char set)
    EscSS3,        // Single shift of form `Esc O <foo>` (G3 char set)
    EscCSIntro,    // Control sequence Intro, `Esc [`
    EscCSParam1,   // first control sequence parameter `[0-9]*;`
    EscCSParam2,   // second control sequence parameter `[0-9]*`
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
    TTY.csParam1 = 1;
    TTY.csParam2 = 1;
    TTY.cursorRow = 0;
    TTY.cursorCol = 0;
    TTY.maxRow = 0;
    TTY.maxCol = 0;
    CURSOR_REQUEST = CursorPosCurrent;
}

void tib_backspace() {
    if(TIB_LEN > 0) {
        TIB[TIB_LEN] = 0;
        TIB_LEN--;
    }
}

void tib_control(unsigned char c) {
    printf(" control(^%c, %u)", c+64, c);
}

void tib_insert(unsigned char c) {
    if(c < ' ' || c == 127 || c > 0xf7) {
        printf(" tib_insert_oor(%u)", c);
        return;
    }
    if(TIB_LEN + 1 >= BUF_SIZE) {
        // Buffer is full
        printf(" tib_insert_full(%c)", c);
    } else {
        TIB[TIB_LEN] = c;
        TIB_LEN++;
        TIB[TIB_LEN] = 0;
    }
}

void tib_cr() {
    printf(" CR\n");
    TIB_LEN = 0;
    TIB[TIB_LEN] = 0;
}

void tty_cs_tilde(uint32_t n) {
    if(n == 2) {
        printf(" Ins");
    } else if(n == 3) {
        printf(" Del");
    } else if(n == 5) {
        printf(" PgUp");
    } else if(n == 6) {
        printf(" PgDn");
    } else if(n == 15) {
        printf(" F5");
    } else if(n == 17) {
        printf(" F6");
    } else if(n == 18) {
        printf(" F7");
    } else if(n == 19) {
        printf(" F8");
    } else if(n == 20) {
        printf(" F9");
    } else if(n == 21) {
        printf(" F10");
    } else if(n == 23) {
        printf(" F11");
    } else if(n == 24) {
        printf(" F12");
    } else {
        printf(" csTilde(%u)", n);
    }
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
        break;
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
        break;
    case CursorPosMaxPossible2:
        // Save the maximum row and column
        TTY.maxRow = row;
        TTY.maxCol = col;
        // Restore original cursor position. This needs CursorPosMaxPossible1
        // to have already saved the original cursor position.
        tty_cursor_goto(TTY.cursorRow, 1);
        // Print the results
        printf("Terminal Size: %u rows, %u cols\n", TTY.maxRow, TTY.maxCol);
        // End the request
        CURSOR_REQUEST = CursorPosCurrent;
        break;
    }
}

void tty_state_normal() {
    TTY.state = Normal;
    TTY.csParam1 = 0;
    TTY.csParam2 = 0;
}

void tty_update_line() {
    char *erase_line = "\33[2K\33[G";  // erase whole line, move to column 1
    write(STDOUT_FILENO, erase_line, 7);
    write(STDOUT_FILENO, TIB, TIB_LEN);
}

void step_tty_state(unsigned char c) {
    switch(TTY.state) {
    case Normal:
        if(c == 13) {  // Enter key is CR (^M) since ICRNL|INLCR are turned off
            tib_cr();
        } else if(c == 27) {
            TTY.state = Esc;
        } else if(c < ' ') {
            tib_control(c);
        } else if(c == 127) {
            tib_backspace();
            tty_update_line();
        } else if(c <= 0xf7) {
            tib_insert(c);
            tty_update_line();
        } else {
            printf(" Normal(%u)", c);
        }
        break;
    case Esc:
        if(c == 'N') {
            TTY.state = EscSS2;
        } else if(c == 'O') {
            TTY.state = EscSS3;
        } else if(c == '[') {
            TTY.state = EscCSIntro;
        } else if(c == 27) {
            TTY.state = Esc;
        } else {
            if(c == 9) {
                printf(" Meta-Tab");
            } else if(c == '(') {
                printf(" Meta-Del");
            } else if(c == 'b') {
                printf(" Meta-Left");
            } else if(c == 'f') {
                printf(" Meta-Right");
            } else if(c == 'F') {
                printf(" Meta-Ins");
            } else if(c == 127) {
                printf(" Meta-Backspace");
            } else if(c == 5) {
                printf(" Meta-Shift-Ins");
            } else {
                printf(" Esc(%c, %u)", c, c);
            }
            tty_state_normal();
        }
        break;
    case EscSS2:
        // TODO: deal with this
        printf(" SS2(%c, %u)", c, c);
        tty_state_normal();
        break;
    case EscSS3:
        if(c == 'P') {
            printf(" F1");
        } else if(c == 'Q') {
            printf(" F2");
        } else if(c == 'R') {
            printf(" F3");
        } else if(c == 'S') {
            printf(" F4");
        } else {
            printf(" SS3(%c, %u)", c, c);
        }
        tty_state_normal();
        break;
    case EscCSIntro:
        if(c >= '0' && c <= '9') {
            TTY.csParam1 = c - '0';
            TTY.state = EscCSParam1;
        } else if (c == ';') {
            TTY.csParam1 = 1;
            TTY.csParam2 = 0;
            TTY.state = EscCSParam2;
        } else {
            if(c == 'A') {
                printf(" Up");
            } else if(c == 'B') {
                printf(" Down");
            } else if(c == 'C') {
                printf(" Right");
            } else if(c == 'D') {
                printf(" Left");
            } else if(c == 'F') {
                printf(" End");
            } else if(c == 'H') {
                printf(" Home");
            } else if(c == 'Z') {
                printf(" Shift-Tab");
            } else {
                printf(" CSIntro(%c, %u)", c, c);
            }
            tty_state_normal();
        }
        break;
    case EscCSParam1:
        if(c >= '0' && c <= '9') {
            TTY.csParam1 *= 10;
            TTY.csParam1 += c - '0';
        } else if(c == ';') {
            TTY.csParam2 = 0;
            TTY.state = EscCSParam2;
        } else {
            if(c == '~') {
                tty_cs_tilde(TTY.csParam1);
            } else {
                printf(" CSParam1(%c, %u)", c, c);
            }
            tty_state_normal();
        }
        break;
    case EscCSParam2:
        if(c >= '0' && c <= '9') {
            TTY.csParam2 *= 10;
            TTY.csParam2 += c - '0';
        } else {
            if(c == 'C') {
                printf(" Shift-Right");
            } else if(c == 'D') {
                printf(" Shift-Left");
            } else if(c == '~') {
                printf(" Shift-Del");
            } else if(c == 'R') {
                // Got cursor position report as answer to a "\33[6n" request
                tty_cursor_handle_report(TTY.csParam1, TTY.csParam2);
            } else {
                printf(" CSParam2(%c)", c);
            }
            tty_state_normal();
        }
        break;
    default:
        printf(" default(%c, %u)", c, c);
    }
}

int main() {
    set_terminal_for_raw_unbuffered_input();
    cold_boot();
    tty_cursor_request_position(CursorPosMaxPossible1);
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
}
