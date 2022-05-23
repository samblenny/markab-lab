( === tests/fizzbuzz.fs ===)
( for integers 1..n, print:            )
(  n divisible by 3       -> "Fizz"    )
(  n divisible by 5       -> "Buzz"    )
(  n divisible by both    -> "FizzBuzz")
(  n divisible by neither -> n         )
( for `0=`, `=`, etc, true is 0, false is 0xffffffff)
( `factors` pushes 1 for divisible by 3, 2 for 5, and 3 for 15)
: if-eq-push-1 = invert 1 and ;
: if-1-f  1 if-eq-push-1 >r  next ."  Fz"   r> drop ;
: if-2-b  2 if-eq-push-1 >r  next ."  Bz"   r> drop ;
: if-3-fb 3 if-eq-push-1 >r  next ."  FzBz" r> drop ;
: if-0-.  0 if-eq-push-1 >r  next .         r> drop ;
: factors dup  3 mod 0= invert 1 and  swap 5 mod 0= invert 2 and  or ;
: fbz-inner dup factors  dup if-1-f  dup if-2-b  dup if-3-fb  if-0-. ;
: n! 0 ! ;
: n@ 0 @ ;
: fbz dup n! >r space  : fbz_ next n@ i - fbz-inner fbz_ ;
( 1 2 Fz 4 Bz Fz 7 8 Fz Bz 11 Fz 13 14 FzBz 16  OK) 16 fbz
