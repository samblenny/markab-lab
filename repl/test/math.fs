( === test/math.fs ===)
reset
( ** Arithmetic **)
( 6  OK)                        3  3 + .
( 1  OK)                        6  5 - .
( -19  OK)                      1 20 - .
( 19  OK)                     -19 -1 * .
( ** Division and modulo division **)
( E12...)                          1 0 /
( E12...)                          1 0 %
( E12...)                         1 0 /%
( 2  OK)                  reset 9 4 / .s
( 1  OK)                  reset 9 4 % .s
( 1 2 OK)                reset 9 4 /% .s
( * Negative dividends are here to check that division operations have  *)
( * been implemented correctly with the right cdq and idiv instructions *)
( * for the proper width of sign extension. Messing up sign extension   *)
( * during division can raise a floating-point exception, so I want to  *)
( * make very sure that won't happen.                                   *)
( -1  OK)                       -9 7 / .
( -2  OK)                       -9 7 % .
( -1 -2  OK)                 -9 7 /% . .
( ** Parsing and overflow behavior for 32-bit signed integers **)
( 2147483647  OK) hex 7fffffff decimal .
( 3FFF0001  OK)        hex 7fff 7fff * .
( E6...)                   hex 1ffffeeee
( 0  OK)                  ffffffff 1 + .
( 1 OK)                 7fffffff dup * .
( FFFFFFFF  OK)         decimal -1 hex .
( ** Two's complement negation with `0 swap -` **)
( 1  OK)           decimal -1 0 swap - .
( 0  OK)                    0 0 swap - .
( FFFFFFFF  OK)         hex 1 0 swap - .
( -2  OK)           decimal 2 0 swap - .
( ** One's complement negation with `~` **)
( FFFFFFFF  OK)                hex 0 ~ .
( 0  OK)              decimal -1 hex ~ .
( FFFFFFFE  OK)                    1 ~ .
( ** Behavior of negative number parsing depends on hex or decimal **)
( FF  OK)                   hex ff 0 + .
(  E7...)           hex -1 0 + decimal .
( -1  OK)               decimal -1 0 + .
( ** Shorthand plus and minus combo words for common increments **)
( 6  OK)  5 1+ .
( 7  OK)  5 2+ .
( 9  OK)  5 4+ .
( 4  OK)  5 1- .
