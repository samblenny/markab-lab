( This is the Markab loadscreen)
."  __  __          _        _" cr
." |  \/  |__ _ _ _| |____ _| |__" cr
." | |\/| / _` | '_| / / _` | '_ \" cr
." |_|  |_\__,_|_| |_\_\__,_|_.__/" cr
: , here ! 4 allot ; ( store T at end of dictionary)
: ? @ . ;         ( fetch address T and print value)
: set-type here b! 1 allot ;  ( set .type field of dictionary item, 8-bit)
: set-head 4 allot last w! ;  ( update pointer to head of dictionary)
: variable here create tpvar set-type 0 here ! ( init to 0) set-head ;
: constant here swap create tpconst set-type here ! ( store n) set-head ;
