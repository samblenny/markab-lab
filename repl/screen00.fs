( This is the Markab loadscreen)
."  __  __          _        _" cr
." |  \/  |__ _ _ _| |____ _| |__" cr
." | |\/| / _` | '_| / / _` | '_ \" cr
." |_|  |_\__,_|_| |_\_\__,_|_.__/" cr
: , here ! 4 allot ; ( store T at end of dictionary)
: ? @ . ;         ( fetch address T and print value)
: setType here b! 1 allot ;  ( set 8-bit .type field of dictionary item)
: setParam here ! 4 allot ;  ( set 32-bit const|var .param field value)
: setHead last w! ;          ( set new head of dictionary)
: var here create TpVar setType 0 ( init to 0) setParam setHead ;
: const here swap create TpConst setType ( T: n) setParam setHead ;
