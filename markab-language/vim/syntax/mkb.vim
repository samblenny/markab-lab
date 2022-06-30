" Copyright (c) 2022 Sam Blenny
" SPDX-License-Identifier: MIT
"
" Vim syntax highlighting for the Markab programming language
"
:syntax case match
:syntax keyword mkbOpcode NOP ADD SUB INC DEC MUL AND INV OR XOR
:syntax keyword mkbOpcode SLL SRL SRA
:syntax keyword mkbOpcode EQ GT LT NE ZE TRUE FALSE JMP JAL CALL RET
:syntax keyword mkbOpcode BZ BFOR MRT MTR RDROP R PC ERR DROP DUP OVER SWAP
:syntax keyword mkbOpcode U8 U16 I32 LB SB LH SH LW SW RESET FENCE CLERR
:syntax keyword mkbOpcode IOD IOR IODH IORH IOKEY IOEMIT IODOT IODUMP TRON TROFF
:syntax keyword mkbOpcode MTA LBA LBAI      AINC ADEC A
:syntax keyword mkbOpcode MTB LBB LBBI SBBI BINC BDEC B
:syntax match mkbComment /( [^)]*)/

" Extend the characters that vim considers acceptable for keywords.
" note: @ is special to vim, but spelling it as @-@ works
:setlocal iskeyword+=+,-,*,64,<,>,{,},@-@,!,=,:,;

:syntax keyword mkbCoreVocab nop + - 1+ 1- * and inv or xor
:syntax keyword mkbCoreVocab << >> >>>
:syntax keyword mkbCoreVocab = > < != 0= true false call
:syntax keyword mkbCoreVocab r> >r rdrop r pc err drop dup over swap
:syntax keyword mkbCoreVocab @ ! h@ h! w@ w! reset fence clerr
:syntax keyword mkbCoreVocab iod ior iodh iorh key emit . dump tron troff
:syntax keyword mkbCoreVocab >a @a @a+     a+ a- a
:syntax keyword mkbCoreVocab >b @b @b+ !b+ b+ b- b >x x >y y
:syntax keyword mkbCoreVocab : ; var const opcode
:syntax keyword mkbCoreVocab if{ }if for{ }for

:highlight link mkbOpcode Constant
:highlight link mkbComment Comment
:highlight link mkbCoreVocab Statement
