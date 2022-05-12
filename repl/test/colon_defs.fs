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
(  OK)                 hex : i 7fffffff ffff ff 1 0 .s ;
( 2147483647 65535 255 1 0  OK)     decimal clearstack h
( 2147483647 65535 255 1 0  OK)             clearstack i
