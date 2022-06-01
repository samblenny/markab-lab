( === test/control_flow.fs ===)
clearstack
(  OK) : zero? if ."   Y" else ."   N" then ." ." ;
(  E1...)                                     zero?
(  Y.  OK)                                  0 zero?
(  N.  OK)                                  1 zero?
(  N.  OK)                                 -1 zero?
(  N.  OK)                                 99 zero?
(  OK)       : red space if ."  Blue" then ."  x" ;
(  Blue  x  OK)                               0 red
(  x  OK)                                     1 red
