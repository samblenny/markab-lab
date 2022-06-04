( === test/stack.fs ===)
reset
( 1  OK)     1    .s
( 1 1  OK)   dup  .s
( 1  OK)     drop .s
( 1 2  OK)   2    .s
( 2 1  OK)   swap .s
( 2 1 2  OK) over .s
reset
( error)      dup
( error)     drop
( error)     swap
( error)     over
reset
( --- These demonstrate that, while there are 17 stack slots, ---)
( --- processing input text needs 2 of those to be left free. ---)
(  E2...)  1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17
(  OK)        1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
(  E2...)                                         .s
(  OK)           1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
(  1 2 ... 14 15  OK)                             .s
(   This works -- 2 free slots for parsing and .s ^^)
(  ... 13 15 14  OK)       swap .s
(  ... 13 15 14  OK)       drop .s
(  ... 13 15 13  OK)       over .s
(  OK)                         dup
(  E2...)                       .s
reset
( Stack is empty  OK) .s
