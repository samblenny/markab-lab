( === test/colon_defs.fs ===)
(  OK)    : a ;               a
( B  OK)  : b ."  B" ;        b
(  OK)    : c ( comment) ;    c
( 3  OK)  : d 1 2 * 1 +  . ;  d
( error)  :
( error)  : e
( error)  : ;
(  OK)       decimal : f -2147483647 -32768 -128 -1 .s ;
(  OK)  hex : g 80000001 ffff8000 ffffff80 ffffffff .s ;
( -2147483647 -32768 -128 -1  OK)   decimal clearstack f
( -2147483647 -32768 -128 -1  OK)           clearstack g
(  OK)         decimal : h 2147483647 65535 255 1 0 .s ;
(  OK)                 hex : k 7fffffff ffff ff 1 0 .s ;
(  2147483647 65535 255 1 0  OK)    decimal clearstack h
(  2147483647 65535 255 1 0  OK)            clearstack k
( I'm a train  OK)           : 🚆 ."  I'm a train" ;  🚆
( --- Test nested function calls ---)
(  OK)       : f0 swap dup . over + ;
(  OK)                : f1 f0 f0 f0 ;
(  OK)       : fib 1 dup f1 f1 f1 . ;
( 1 1 2 3 5 8 13 21 34 55 89  OK) fib
clearstack
( --- Test implicit tail call ---)
(  OK) : start 1 .  : continue 2 . 3 . ;
( 1 2 3  OK)                       start
( 2 3  OK)                      continue
clearstack
( --- Test maxing out return stack ---)
(  OK)  : rs0 0 space : rs1 1 + 42 emit rs1 nop ;
(     nop here stops tail call optimization ^^^ )
( ***************** E21...)        clearstack rs0
( ***************** E21...)                   rs0
( ^^ note auto-recovery from full return stack)
(  18 18  OK)                                  .s
( --- Test tail call optimization ---)
(  OK)   : tc0 0 . ;  : tc1 1 . tc0 ;
( 1 0  OK)             clearstack tc1
(  OK)       : tc2 0  : tc3 1 + tc3 ;
(       infinite tail recursion ^^^ )
(  E22...)             clearstack tc2
(  174762  OK)                     .s
