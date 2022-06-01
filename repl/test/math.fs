( === test/math.fs ===)
clearstack
( 6  OK)                        3  3 + .
( 1  OK)                        6  5 - .
( -19  OK)                      1 20 - .
( 19  OK)                     -19 -1 * .
( 2147483647  OK) hex 7fffffff decimal .
( 3FFF0001  OK)        hex 7fff 7fff * .
( error)                           1 0 /
( error)                         1 0 mod
( error)                        1 0 /mod
( 2  OK)             clearstack 9 4 / .s
( 1  OK)           clearstack 9 4 mod .s
( 1 2 OK)         clearstack 9 4 /mod .s
( error)                   hex 1ffffeeee
( 0  OK)                  ffffffff 1 + .
( 1 OK)                 7fffffff dup * .
( FFFFFFFF  OK)         decimal -1 hex .
( 0  OK)                      0 negate .
( FFFFFFFF  OK)               0 invert .
( 1  OK)         decimal -1 hex negate .
( 0  OK)         decimal -1 hex invert .
( FFFFFFFF  OK)               1 negate .
( FFFFFFFE  OK)               1 invert .
( FF  OK)           hex ff 0 + . decimal
(  E7...)           hex -1 0 + decimal .
( -1  OK)               decimal -1 0 + .
