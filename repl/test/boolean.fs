( === test/boolean.fs ===)
( ** Forth boolean values, at least in modern standard Forths,  **)
( ** are non-zero for True [typically -1], or zero for False.   **)
( ** This is fairly similar to C. But, I think there have also  **)
( ** been other Forths that flip it around backwards. I started **)
( ** off doing it that way for some reason -- maybe I read some **)
( ** old thing of Chuck's? But, this way is better.             **)
decimal clearstack
( -1 0 0  OK)    1 2 <  .  2 2 <  .  2 1 <  .
( 0 0 -1  OK)    1 2 >  .  2 2 >  .  2 1 >  .
( -1 -1 0  OK)   1 2 <= .  2 2 <= .  2 1 <= .
( 0 -1 -1  OK)   1 2 >= .  2 2 >= .  2 1 >= .
( 0 -1 0  OK)    1 2 =  .  2 2 =  .  2 1 =  .
( -1 0 -1  OK)   1 2 <> .  2 2 <> .  2 1 <> .
( 0 -1 0  OK)     -1 0= .    0 0= .    1 0= .
( -1 0 0  OK)     -1 0< .    0 0< .    1 0< .
: true ."  T" ;
: false ."  F" ;
: if<  <  if true else false endif ;
: if>  >  if true else false endif ;
: if<= <= if true else false endif ;
: if>= >= if true else false endif ;
: if=  =  if true else false endif ;
: if<> <> if true else false endif ;
: if0= 0= if true else false endif ;
: if0< 0< if true else false endif ;
( T F F  OK)  1 2 if<   2 2 if<   2 1  if<
( F F T  OK)  1 2 if>   2 2 if>   2 1  if>
( T T F  OK)  1 2 if<=  2 2 if<=  2 1 if<=
( F T T  OK)  1 2 if>=  2 2 if>=  2 1 if>=
( F T F  OK)  1 2 if=   2 2 if=   2 1  if=
( T F T  OK)  1 2 if<>  2 2 if<>  2 1 if<>
( F T F  OK)   -1 if0=    0 if0=    1 if0=
( T F F  OK)   -1 if0<    0 if0<    1 if0<
