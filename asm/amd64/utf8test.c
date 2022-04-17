#include <stdio.h>
#include <stdint.h>

/*
 * This demonstrates the ranges of values for UTF-8 leading and continuation
 * bytes. Values should be 127 or less for ASCII, or in the range of 128..247
 * for UTF-8 encoded characters.
 *

Output of `make utf8test.run`:
```
10000000 -- 80 -- 128
10111111 -- bf -- 191
11000000 -- c0 -- 192
11011111 -- df -- 223
11100000 -- e0 -- 224
11101111 -- ef -- 239
11110000 -- f0 -- 240
11110111 -- f7 -- 247
```
 */

int main () {
    #define SIZE 8
    uint8_t vals[SIZE] = {
        0b10000000, 0b10111111,  // continuation: 10......
        0b11000000, 0b11011111,  // leading byte: 110..... style
        0b11100000, 0b11101111,  // leading byte: 1110.... style
        0b11110000, 0b11110111,  // leading byte: 11110... style
    };
    for(int i=0; i<SIZE; i++) {
        uint8_t c = vals[i];
        for(int j=0; j<8; j++) {
            printf("%d", ((c<<j) & 128) >> 7);  // since printf doesn't do binary
        }
        printf(" -- %02x -- %03d\n", c, c);
    }
    return 0;
}
