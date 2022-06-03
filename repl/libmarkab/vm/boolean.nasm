; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth boolean words (meant to be included in ../libmarkab.nasm)
;
; == Important Note! ==
; Forth's boolean constants are not like C booleans. In Forth,
;   True value:   0 (all bits clear)
;   False value: -1 (all bits set)
; This allows for sneaky tricks such as using the `AND`, `OR`, `XOR`, and
; `INVERT` to act as both bitwise and boolean operators.
; =====================

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"

extern mErr1Underflow
extern mPopW

global mAnd
global mOr
global mXor
global mInvert
global mLess
global mGreater
global mEqual
global mZeroLess
global mZeroEqual


mAnd:                         ; AND   ( 2nd T -- bitwise_and_2nd_T )
fDo PopW, .end
and T, W
.end:
ret

mOr:                          ; OR   ( 2nd T -- bitwise_or_2nd_T )
fDo PopW, .end
or T, W
.end:
ret

mXor:                         ; XOR   ( 2nd T -- bitwise_xor_2nd_T )
fDo PopW, .end
xor T, W
.end:
ret

mInvert:                      ; Invert all bits of T (one's complement)
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
not T                         ; note amd64 not opcode is one's complement
ret

mLess:                        ; <   ( 2nd T -- bool_is_2nd_less_than_T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to false (-1), assuming 2nd >= T
dec T
xor esi, esi                  ; prepare true (0) in esi
cmp edi, W                    ; test for 2nd < T
cmovl T, esi                  ; if so, change new T to true
.end:
ret

mGreater:                     ; >   ( 2nd T -- bool_is_2nd_greater_than_T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to true (0), assuming 2nd > T
xor esi, esi                  ; prepare false (-1) in esi
dec esi
cmp edi, W                    ; test for 2nd <= T
cmovle T, esi                 ; if so, change new T to false
.end:
ret

mEqual:                       ; =   ( 2nd T -- bool_is_2nd_equal_to_T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to true (0), assuming 2nd = T
xor esi, esi                  ; prepare false (-1) in esi
dec esi
cmp edi, W                    ; test for 2nd <> T   (`<>` means not-equal)
cmovnz T, esi                 ; if so, change new T to false
.end:
ret

mZeroLess:                    ; 0<   ( T -- bool_is_T_less_than_0 )
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
mov W, T
xor T, T                      ; set T to false (-1)
dec T
xor edi, edi                  ; prepare value of true (0) in edi
test W, W                     ; check of old T<0 by setting sign flag (SF)
cmovs T, edi                  ; if so, change new T to true
ret

mZeroEqual:                   ; 0=   ( T -- bool_is_T_equal_0 )
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
mov W, T                      ; save value of T
xor T, T                      ; set T to false (-1)
dec T
xor edi, edi                  ; prepare value of true (0) in edi
test W, W                     ; check if old T was zero (set ZF for W and W)
cmove T, edi                  ; if so, change new T to true
ret

