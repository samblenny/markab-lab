( === test/control_flow.fs ===)
reset
(  OK) : zero? 0= if ."   Y" else ."   N" ;if ." ." ;
(  E1...)                                      zero?
(  Y.  OK)                                   0 zero?
(  N.  OK)                                   1 zero?
(  N.  OK)                                  -1 zero?
(  N.  OK)                                  99 zero?
(  OK)      : red space 0= if ."  Blue" ;if ."  x" ;
(  Blue  x  OK)                                0 red
(  x  OK)                                      1 red
