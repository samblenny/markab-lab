( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( === THIS FILE IS AUTOMATICALLY GENERATED ===)
( ===        DO NOT MAKE EDITS HERE        ===)
( ===      See codegen.py for details      ===)

( MarkabVM virtual CPU opcode tokens)
 0 const Nop
 1 const ADD
 2 const SUB
 3 const MUL
 4 const AND
 5 const INV
 6 const OR
 7 const XOR
 8 const SHL
 9 const SHR
10 const SHA
11 const EQ
12 const GT
13 const LT
14 const NE
15 const ZE
16 const JMP
17 const CALL
18 const RET
19 const JZ
20 const DRJNN
21 const RFROM
22 const TOR
23 const RESET
24 const DROP
25 const DUP
26 const OVER
27 const SWAP
28 const U8
29 const U16
30 const I32
31 const BFET
32 const BSTO
33 const WFET
34 const WSTO
35 const FET
36 const STO

( MarkabForth core vocabulary)
: nop   tok> Nop ;
: +     tok> ADD ;
: -     tok> SUB ;
: *     tok> MUL ;
: &     tok> AND ;
: ~     tok> INV ;
: |     tok> OR ;
: ^     tok> XOR ;
: <<    tok> SHL ;
: >>    tok> SHR ;
: >>>   tok> SHA ;
: =     tok> EQ ;
: >     tok> GT ;
: <     tok> LT ;
: <>    tok> NE ;
: 0=    tok> ZE ;
: ;     tok> RET ;
: r>    tok> RFROM ;
: >r    tok> TOR ;
: reset tok> RESET ;
: drop  tok> DROP ;
: dup   tok> DUP ;
: over  tok> OVER ;
: swap  tok> SWAP ;
: b@    tok> BFET ;
: b!    tok> BSTO ;
: w@    tok> WFET ;
: w!    tok> WSTO ;
: @     tok> FET ;
: !     tok> STO ;
