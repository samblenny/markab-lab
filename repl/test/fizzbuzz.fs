( === tests/fizzbuzz.fs ===)
clearstack
( for integers 1..n, print:            )
(  n divisible by 3       -> "Fizz"    )
(  n divisible by 5       -> "Buzz"    )
(  n divisible by both    -> "FizzBuzz")
(  n divisible by neither -> n         )
( for `0=`, `=`, etc, true is 0, false is 0xffffffff)
( `factors` pushes 1 for divisible by 3, 2 for 5, and 3 for 15)
: if-eq-push-1 = invert 1 and ;
: if-1-f  1 = invert if ."  Fz" endif ;
: if-2-b  2 = invert if ."  Bz" endif ;
: if-3-fb 3 = invert if ."  FzBz" endif ;
: if-0-.  0= invert if . endif ;
: factors dup  3 mod 0= 1 and  swap 5 mod 0= 2 and  or ;
: fbz-inner dup factors  dup if-1-f  dup if-2-b  dup if-3-fb  if-0-. ;
: n! 60000 ! ;
: n@ 60000 @ ;
: fbz space dup n! 1 - for n@ i - fbz-inner next ;
( 1 2 Fz 4 Bz Fz 7 8 Fz Bz 11 Fz 13 14 FzBz 16  OK) 16 fbz
