" Copyright (c) 2022 Sam Blenny
" SPDX-License-Identifier: MIT
"
" Vim syntax highlighting for the Markab programming language
"
:syntax case match
:syntax keyword mkbOpcode NOP ADD SUB MUL AND INV OR XOR SLL SRL SRA
:syntax keyword mkbOpcode EQ GT LT NE ZE JMP JAL RET
:syntax keyword mkbOpcode BZ DRBLT MRT MTR RDROP DROP DUP OVER SWAP
:syntax keyword mkbOpcode U8 U16 I32 LB SB LH SH LW SW RESET ECALL
:syntax keyword mkbEcallCode E_DS E_DSH E_RS E_RSH E_PC E_READ E_WRITE
:syntax match mkbComment /( [^)]*)/
:syntax match mkbChar /'[^']'/

" Extend the characters that vim considers acceptable for keywords:
"  skip | since it is special to vim, it gets handled with a match instead
"  ^ is special, but spelling it as ASCII 64 works
"  @ is also special, but spelling it as @-@ works
:setlocal iskeyword+=+,-,*,&,~,64,<,>,{,},@-@,!,=,:,;

" | and ^ are special, so handle them with match instead of keyword
:syntax match mkbCoreVocab /|/
:syntax match mkbCoreVocab /\^/

:syntax keyword mkbCoreVocab nop + - * & ~
:syntax keyword mkbCoreVocab << >> >>> = > < != 0= : ; var const
:syntax keyword mkbCoreVocab r> >r rdrop drop dup over swap
:syntax keyword mkbCoreVocab b@ b! h@ h! w@ w!
:syntax keyword mkbCoreVocab if{ }if for{ }for ASM{ }ASM

:highlight link mkbOpcode Constant
:highlight link mkbEcallCode Constant
:highlight link mkbComment Comment
:highlight link mkbChar Character
:highlight link mkbCoreVocab Statement
