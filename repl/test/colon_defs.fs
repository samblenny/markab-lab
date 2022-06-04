( === test/colon_defs.fs ===)
(  OK)    : a ;               a
( B  OK)  : b ."  B" ;        b
(  OK)    : c ( comment) ;    c
( 3  OK)  : d 1 2 * 1 +  . ;  d
( error)  :
( error)  : e
( error)  : ;
( error)  : a  : b ;
(  OK)       decimal : f -2147483647 -32768 -128 -1 .s ;
(  OK)  hex : g 80000001 ffff8000 ffffff80 ffffffff .s ;
( -2147483647 -32768 -128 -1  OK)        decimal reset f
( -2147483647 -32768 -128 -1  OK)                reset g
(  OK)         decimal : h 2147483647 65535 255 1 0 .s ;
(  OK)                 hex : k 7fffffff ffff ff 1 0 .s ;
(  2147483647 65535 255 1 0  OK)         decimal reset h
(  2147483647 65535 255 1 0  OK)                 reset k
( I'm a train  OK)           : 🚆 ."  I'm a train" ;  🚆
( --- Test nested function calls ---)
(  OK)       : f0 swap dup . over + ;
(  OK)                : f1 f0 f0 f0 ;
(  OK)       : fib 1 dup f1 f1 f1 . ;
( 1 1 2 3 5 8 13 21 34 55 89  OK) fib
( --- Test maxing out return stack ---)
(  OK)  : rs1 1 + 42 emit rs1 nop ; : rs0 0 space rs1 ;
(                    nop here ^^^ stops tail call optimization)
( ***************** E21...)             reset rs0
( ***************** E21...)                   rs0
( ^^ note auto-recovery from full return stack)
( Stack is empty  OK)                        .ret
( Stack is empty  OK)                          .s
reset
( --- Test tail call optimization ---)
(  OK)   : tc0 0 . ;  : tc1 1 . tc0 ;
( 1 0  OK)                  reset tc1
(  OK)  : tc3 1 + tc3 ; : tc2 0 tc3 ;
(                 ^^^ infinite tail recursion)
(  E22...)                  reset tc2
(  174762  OK)                     .s
