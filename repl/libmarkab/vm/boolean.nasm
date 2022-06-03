; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth boolean words (meant to be included in ../libmarkab.nasm)
;
; == Important Note! ==
; MarkabForth's boolean truth values (same as standard Forths):
;   True:  -1 (all bits set)
;   False: 0 (all bits clear)
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
global mLessEq
global mGreater
global mGreaterEq
global mEqual
global mNotEq
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

mLess:                        ; <   ( 2nd T -- boolean: 2nd < T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor esi, esi                  ; prepare true (-1) in esi
dec esi
xor T, T                      ; new T = false (0), assuming 2nd >= T
cmp edi, W                    ; test for 2nd < T
cmovl T, esi                  ; if so, change new T to true
.end:
ret

mLessEq:                      ; <=   ( 2nd T -- boolean: 2nd <= T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor esi, esi                  ; prepare true (-1) in esi
dec esi
xor T, T                      ; new T = false (0), assuming 2nd >= T
cmp edi, W                    ; test for 2nd <= T
cmovle T, esi                 ; if so, change new T to true
.end:
ret

mGreater:                     ; >   ( 2nd T -- boolean: 2nd > T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor esi, esi                  ; prepare true (-1) in esi
dec esi
xor T, T                      ; new T = false (0), assuming 2nd <= T
cmp edi, W                    ; test for 2nd > T
cmovg T, esi                  ; if so, change new T to true
.end:
ret

mGreaterEq:                   ; >   ( 2nd T -- boolean: 2nd >= T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor esi, esi                  ; prepare true (-1) in esi
dec esi
xor T, T                      ; new T = false (0), assuming 2nd <= T
cmp edi, W                    ; test for 2nd >= T
cmovge T, esi                 ; if so, change new T to true
.end:
ret

mEqual:                       ; =   ( 2nd T -- boolean: 2nd == T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor esi, esi                  ; prepare true (-1) in esi
dec esi
xor T, T                      ; new T = false (0), assuming 2nd <> T
cmp edi, W                    ; test for 2nd == T
cmove T, esi                  ; if so, change new T to true
.end:
ret

mNotEq:                       ; =   ( 2nd T -- boolean: 2nd <> T )
fDo PopW, .end
mov edi, T                    ; save value of old 2nd in edi
xor esi, esi                  ; prepare true (-1) in esi
dec esi
xor T, T                      ; new T = false (0), assuming 2nd <> T
cmp edi, W                    ; test for 2nd <> T
cmovne T, esi                 ; if so, change new T to true
.end:
ret

mZeroLess:                    ; 0<   ( T -- bool_is_T_less_than_0 )
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
mov W, T
xor T, T                      ; set T to false (0)
xor edi, edi                  ; prepare value of true (-1) in edi
dec edi
test W, W                     ; check of old T<0 by setting sign flag (SF)
cmovs T, edi                  ; if so, change new T to true
ret

mZeroEqual:                   ; 0=   ( T -- bool_is_T_equal_0 )
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
mov W, T                      ; save value of T
xor T, T                      ; set T to false (0)
xor edi, edi                  ; prepare value of true (-1) in edi
dec edi
test W, W                     ; check if old T was zero (set ZF for W and W)
cmove T, edi                  ; if so, change new T to true
ret
