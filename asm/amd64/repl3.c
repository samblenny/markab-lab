#include <limits.h>   // CHAR_BIT
#include <stdint.h>   // uint8_t
#include <stdlib.h>   // atexit
#include <stdio.h>    // printf
#include <termios.h>  // termios, tcsetattr, tcsgetattr, tcflag_t, ECHO,
                      // ICANON, TCSANOW, ...
#include <unistd.h>   // read, write, STDIN_FILENO

/*
 * This improves on repl2 mainly by making sure to pass utf-8 encoded bytes to
 * the output stream properly.
 *

Output from `make repl3.run` with pasted unicode stuff, then typed left-arrow,
right-arrow, a, b, control-c:
```
Ready
read 9: e3 81 84 e3 82 8d e3 81 af  いろは
read 4: f0 9f 9a 85  🚅
read 4: f0 9f a6 91  🦑
read 13: f0 9f 8f b4 e2 80 8d e2 98 a0 ef b8 8f  🏴‍☠️
read 3: 1b 5b 44   Esc [ D
read 3: 1b 5b 43   Esc [ C
read 1: 61   a
read 1: 62   b
read 1: 03   ^C
```
 */


struct termios old_config;

void set_old_config() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_config);
}

int main() {
    // Use POSIX termios function to save initial terminal config, then set a
    // hook to restore it as this process exits
    struct termios new_config;
    tcgetattr(STDIN_FILENO, &old_config);
    atexit(set_old_config);

    // Set up a new config for raw, character-buffered input. This turns off
    // various buffering and pre-processing features. The goal is to pass as
    // many control characters as possible through to read() so they can be
    // used as editor keyboard shortcuts. Unless I messed up, these flags
    // should all be standard POSIX termios stuff.
    tcflag_t imask = ~(IXON|IXOFF|ISTRIP|INLCR|IGNCR|ICRNL);
    tcflag_t Lmask = ~(ICANON|ECHO|ECHONL|ISIG|IEXTEN);
    new_config = old_config;
    new_config.c_iflag &= imask;
    new_config.c_lflag &= Lmask;
    new_config.c_cflag |= CS8;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_config);

    // Read and decode some input characters
    printf("Ready\n");
    for(int i=0; i<99; i++) {
        const size_t BUF_SIZE = 64;
        unsigned char buf[BUF_SIZE];
        int chars_read = read(STDIN_FILENO, buf, BUF_SIZE);
        printf("read %d:", chars_read);
        if(chars_read == 0) {
            // Break for end of file
            printf(" EOF\n");
            break;
        }
        for(int j=0; j<chars_read && j<BUF_SIZE; j++) {
            printf(" %02x", buf[j]);
        }
        printf("  ");
        // Pretty print ASCII chars, control chars, and escape sequences
        for(int j=0; j<chars_read && j<BUF_SIZE; j++) {
            unsigned char c = buf[j];
            if(c == ' ') {
                printf(" Spc");
            } else if(c == 27) {
                printf(" Esc");
            } else if(c < ' ') {
                printf(" ^%c", c + 64); // 1=>"^A", 2=>"^B", Esc=>"^[", ...
            } else if(c < 127) {
                printf(" %c", c);       // ASCII printable
            } else if(c == 127) {
                printf(" Del");
            } else if(c <= 0xf7) {
                printf("%c", c);        // UTF-8
            } else {
                printf( " --");         // Invalid UTF-8 bytes
            }
        }
        printf("\n");
        // Break for ^C
        if((chars_read == 1) && (buf[0] == 'C'-64)) {
            break;
        }
    }
    return 0;
}
