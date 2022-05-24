( === tests/return_stack.fs ===)
(  OK)  : a 1 >r 2 >r 3 >r .ret r> r> r> ;
(  1 2 3  OK)                            a
(  OK)                : broken 1 >r .ret ;
(  E19 Bad address)                 broken
(   Stack is empty  OK)               .ret
( >r should not be defined when not compiling)  1 >r
( ------------------------ TODO: also fix this! ^^^)
