" Copyright (c) 2022 Sam Blenny
" SPDX-License-Identifier: MIT
"
" Vim syntax highlighting for the Markab programming language
"
:syntax case match
:syntax keyword mkbOpcode NOP ADD SUB INC DEC MUL DIV MOD AND INV OR XOR
:syntax keyword mkbOpcode SLL SRL SRA
:syntax keyword mkbOpcode EQ GT LT NE ZE TRUE FALSE JMP JAL CALL RET HALT
:syntax keyword mkbOpcode BZ BFOR MTR RDROP R PC MTE DROP DUP OVER SWAP
:syntax keyword mkbOpcode U8 U16 I32 LB SB LH SH LW SW RESET
:syntax keyword mkbOpcode IOD IODH IORH IOKEY IOEMIT IODOT IODUMP TRON TROFF
:syntax keyword mkbOpcode IOLOAD IOSAVE
:syntax keyword mkbOpcode MTA LBA LBAI      AINC ADEC A
:syntax keyword mkbOpcode MTB LBB LBBI SBBI BINC BDEC B
:syntax match mkbComment /( [^)]*)/

" Extend the characters that vim considers acceptable for keywords.
" note: @ is special to vim, but spelling it as @-@ works
:setlocal iskeyword+=+,-,*,/,%,64,<,>,{,},@-@,!,=,:,;

:syntax keyword mkbCoreVocab nop + - 1+ 1- * / % and inv or xor
:syntax keyword mkbCoreVocab << >> >>>
:syntax keyword mkbCoreVocab = > < != 0= true false call halt
:syntax keyword mkbCoreVocab >r rdrop r pc >err drop dup over swap
:syntax keyword mkbCoreVocab @ ! h@ h! w@ w! reset
:syntax keyword mkbCoreVocab iod iodh iorh key emit . dump tron troff
:syntax keyword mkbCoreVocab load save
:syntax keyword mkbCoreVocab >a @a @a+     a+ a- a
:syntax keyword mkbCoreVocab >b @b @b+ !b+ b+ b- b >x x >y y
:syntax keyword mkbCoreVocab : ; var const opcode
:syntax keyword mkbCoreVocab if{ }if for{ }for

:highlight link mkbOpcode Constant
:highlight link mkbComment Comment
:highlight link mkbCoreVocab Statement
