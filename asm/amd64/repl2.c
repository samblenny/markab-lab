#include <limits.h>   // CHAR_BIT
#include <stdint.h>   // uint8_t
#include <stdlib.h>   // atexit
#include <stdio.h>    // printf
#include <termios.h>  // termios, tcsetattr, tcsgetattr, tcflag_t, ECHO, ICANON,
                      // TCSANOW
#include <unistd.h>   // read, write, STDIN_FILENO

/*
 * Output from `make repl2.run` with typed input of:
 *   q w e Ctrl-q Ctrl-w Ctrl-e Alt-q Alt-w Alt-e Up Down Left Right Backspace
 *   Del Enter Space Ctrl-C

$ make repl2.run
CHAR_BIT: 8
sizeof(struct termios): 60
sizeof(tcflag_t): 4
sizeof(termios.c_iflag): 4
read 1: 113   q
read 1: 119   w
read 1: 101   e
read 1: 17   ^Q
read 1: 23   ^W
read 1: 5   ^E
read 2: 27 113   Esc q
read 2: 27 119   Esc w
read 2: 27 101   Esc e
read 3: 27 91 65   Esc [ A
read 3: 27 91 66   Esc [ B
read 3: 27 91 68   Esc [ D
read 3: 27 91 67   Esc [ C
read 1: 127   127
read 4: 27 91 51 126   Esc [ 3 ~
read 1: 10   ^J
read 1: 32   Spc
read 1: 3   ^C
 */

struct termios old_config;

void set_old_config() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_config);
}

int main() {
    // Print sizeof stats for termio struct to help with assembly CALLs
    printf("CHAR_BIT: %d\n", CHAR_BIT);
    printf("sizeof(struct termios): %ld\n", sizeof(struct termios));
    printf("sizeof(tcflag_t): %ld\n", sizeof(tcflag_t));
    printf("sizeof(termios.c_iflag): %ld\n", sizeof(old_config.c_iflag));

    // Save old terminal config and set an exit hook to restore it, so that
    // the shell hopefully doesn't get confused
    // 1. Display sizeof stats for termios struct
    // 2. Get the original tty configuration
    // 3. Clear canonical mode bit to turn off input line-buffering
    // 4. Clear keystroke echo bit
    // 5. Set the modified tty configuration
    // 6. Read characters
    // 7. Restore original tty configuration
    struct termios new_config;
    tcgetattr(STDIN_FILENO, &old_config);
    atexit(set_old_config);

    // Set up a new config for character-buffered input
    new_config = old_config;
    new_config.c_iflag &= ~(IXON|IXOFF);   // no XON/XOFF flow control
    new_config.c_lflag &= ~(ICANON|ECHO);  // no input line buffering
    new_config.c_lflag &= ~(ISIG);  // don't send signals for ^C, ^Z, ...

    tcsetattr(STDIN_FILENO, TCSANOW, &new_config);

    // Read and decode some input characters
    for(int i=0; i<99; i++) {
        const size_t BUF_SIZE = 8;
        char input_buffer[BUF_SIZE];
        int chars_read = read(STDIN_FILENO, input_buffer, BUF_SIZE);
        printf("read %d:", chars_read);
        for(int j=0; j<chars_read && j<BUF_SIZE; j++) {
            printf(" %u", input_buffer[j]);
        }
        printf("  ");
        for(int j=0; j<chars_read && j<BUF_SIZE; j++) {
            char c = input_buffer[j];
            if(c == 27) {
                printf(" Esc");
            } else if(c == ' ') {
                printf(" Spc");
            } else if(c < ' ') {
                printf(" ^%c", c + 64); // 1=>"^A", 2=>"^B", ...
            } else if(c >= 127) {
                printf(" %d", c);
            } else {
                printf( " %c", c);
            }
        }
        printf("\n");
        // Break for ^C or ^D
        if(chars_read == 1) {
            char c = input_buffer[0];
            if((c=='C'-64) || (c=='D'-64)) {
                break;
            }
        }
        //write(STDOUT, &c, 1);
    }
    return 0;
}
