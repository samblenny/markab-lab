( === test/loops.fs ===)
( the 0; word returns if top of stack is 0)
(  OK)        : 9down 9    : L1 0; 1 - dup .      L1 ;
( 8 7 6 5 4 3 2 1 0  OK)                         9down
(  OK)        : 9up 9      : L2 0; 1 - 8 over - . L2 ;
( 0 1 2 3 4 5 6 7 8  OK)                           9up
(  OK)        : 9under -9  : L3 0; 1 + dup .      L3 ;
( -8 -7 -6 -5 -4 -3 -2 -1 0  OK)                9under
