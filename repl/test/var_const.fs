( === test/var_const.fs ===)
clearstack
( <number>  OK)               here .
(  E15...)               10000 allot
(        This is too big ^^^^^     )
( <same-number>  OK)          here .
(  OK)                       var foo
( <bigger-number>  OK)        here .
(  OK)                      99 foo !
( 99  OK)                    foo @ .
( 99  OK)                      foo ?
( 99  OK)      : .foo foo @ . ; .foo
(  OK)                  20 const bar
( 20  OK)                      bar .
( 20  OK)        : .bar bar . ; .bar
( E13...)                   30 bar !
(  OK)           var array 1 array !
(  OK)                   2 , 3 , 4 ,
( 1 OK)                      array ?
( 2 OK)                  array 4 + ?
( 3 OK)                  array 8 + ?
( 4 OK)                 array 12 + ?
(  OK)     here array - const arrlen
( 16 OK)                    arrlen .
