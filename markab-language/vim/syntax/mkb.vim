" Copyright (c) 2022 Sam Blenny
" SPDX-License-Identifier: MIT
"
" Vim syntax highlighting for the Markab programming language
"
:syntax case match
:syntax keyword mkbOpcode NOP ADD SUB MUL AND INV OR XOR SLL SRL SRA
:syntax keyword mkbOpcode EQ GT LT NE ZE JMP JAL RET
:syntax keyword mkbOpcode BZ DRBLT MRT MTR RDROP R PC DROP DUP OVER SWAP
:syntax keyword mkbOpcode U8 U16 I32 LB SB LH SH LW SW RESET
:syntax keyword mkbOpcode IOD IOR IODH IORH IOKEY IOEMIT
:syntax keyword mkbOpcode MTA LBAI AINC ADEC A
:syntax keyword mkbOpcode MTB SBBI BINC BDEC B MTX X MTY Y
:syntax match mkbComment /( [^)]*)/

" Extend the characters that vim considers acceptable for keywords.
" note: @ is special to vim, but spelling it as @-@ works
:setlocal iskeyword+=+,-,*,64,<,>,{,},@-@,!,=,:,;

:syntax keyword mkbCoreVocab nop + - * and inv or xor
:syntax keyword mkbCoreVocab << >> >>> = > < != 0= : ; var const
:syntax keyword mkbCoreVocab r> >r rdrop r pc drop dup over swap
:syntax keyword mkbCoreVocab @ ! h@ h! w@ w!
:syntax keyword mkbCoreVocab iod ior iodh iorh key emit
:syntax keyword mkbCoreVocab >a @a+ a+ a- a
:syntax keyword mkbCoreVocab >b !b+ b+ b- b >x x >y y
:syntax keyword mkbCoreVocab if{ }if for{ break }for ASM{ }ASM

:highlight link mkbOpcode Constant
:highlight link mkbComment Comment
:highlight link mkbCoreVocab Statement
