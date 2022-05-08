( === test/stack.fs ===)
clearstack
( 1  OK)     1    .s
( 1 1  OK)   dup  .s
( 1  OK)     drop .s
( 1 2  OK)   2    .s
( 2 1  OK)   swap .s
( 2 1 2  OK) over .s
clearstack
( error)      dup
( error)     drop
( error)     swap
( error)     over
clearstack
( OK)    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17
( 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17  OK) .s
( error)          dup
( error)         over
( ... 17 16  OK) swap
( ... 15 17  OK) drop
clearstack
( error) 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
( 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17  OK) .s
clearstack
( Stack is empty) .s
