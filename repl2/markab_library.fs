( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( MarkabForth library: all the stuff that runs on top of the kernel)

: asm8  here [ ByteStore ] ;  ( compile low byte of T as a U8)
: asm16 here [ WordStore ] ;  ( compile low word of T as U16)
: asm32 here [ Store     ] ;  ( compile double word T as U32)
: @  Fetch asm8 ;
: !  Store asm8 ;
: w@ WordFetch asm8 ;
: w! WordStore asm8 ;
: b@ ByteFetch asm8 ;
: b! ByteStore asm8 ;
: +  Plus asm8 ;
: -  Minus asm8 ;
: *  Mul asm8 ;
: /  Div asm8 ;
: %  Mod asm8 ;
: /% DivMod asm8 ;
: &  And asm8 ;
: |  Or asm8 ;
: ^  Xor asm8 ;


: outer ( outer interpreter) ;
: follow @ ;
: find current dup 0= if ret endif w@ ;
: if   ;
: else  ;
: ;if   ;
: for here  ;
: ;for  ;
: .  ;
: hex decimal does> 16 base ! ;
: decimal decimal does> 10 base ! ;

decimal
?? const TpVar
?? const TpConst
?? const TpCode
?? const TpToken
?? const Heap
65535 const HeapEnd
256 const HReserve
var DP
var Base
var Pad
var Blk
var TIB
var CallDP
var Context
var Current
var Last

: here dp @ ;
: allot here + dp ! ;
: , here 4 allot ! ;
: ? @ . ;
: setType here b! 1 allot ;
: setParam here ! 4 allot ;
: setHead last w! ;
: var here create TpVar setType 0 ( init to 0) setParam setHead ;
: const here swap create TpConst setType ( T: n) setParam setHead ;

: ." ... does> ... ;
: version ." markabForth v0.2.0" ;
const
: init-vars ?? Pad ! ?? Blk ! ?? TIB ! ?? CallDP ! Heap Base ;
: cold init-vars version ... ;
: doword type? case 0 doToken 1 doCode 2 doConst 3 doVar endcase ;
: interpret word find if doword interpret ret endif ;
