( === tests/fizzbuzz.fs ===)
reset
( for integers 1..n, print:            )
(  n divisible by 3       -> "Fizz"    )
(  n divisible by 5       -> "Buzz"    )
(  n divisible by both    -> "FizzBuzz")
(  n divisible by neither -> n         )
: buzz? dup  5 % 0= if drop ."  Bz"   else .  ;if ;
: fizz? dup  3 % 0= if drop ."  Fz"   ; ;if buzz? ;
: fzbz? dup 15 % 0= if drop ."  FzBz" ; ;if fizz? ;
(   conditional return from IF branch ^)
(     ...allows for tail call optimizations ^^^^^)
var n
: fbz space dup n ! 1 - for n @ i - fzbz? ;for ;
( 1 2 Fz 4 Bz Fz 7 8 Fz Bz 11 Fz 13 14 FzBz 16  OK) 16 fbz
