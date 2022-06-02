; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth math words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"

extern DSBase
extern mErr12DivideByZero
extern mErr1Underflow
extern mPopW

global mPlus
global mMinus
global mNegate
global mMul
global mDiv
global mMod
global mDivMod
global mMax
global mMin
global mAbs
global mOnePlus
global mTwoPlus
global mFourPlus


mPlus:                        ; +   ( 2nd T -- 2nd+T )
fDo PopW, .end
add T, W
.end:
ret

mMinus:                       ; -   ( 2nd T -- 2nd-T )
fDo PopW, .end
sub T, W
.end:
ret

mNegate:                      ; Negate T (two's complement)
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
neg T
ret

mMul:                         ; *   ( 2nd T -- 2nd*T )
fDo PopW, .end
imul T, W                     ; imul is signed multiply (mul is unsigned)
.end:
ret

mDiv:                         ; /   ( 2nd T -- <quotient 2nd/T> )
fDo PopW, .end
test W, W                     ; make sure divisor is not 0
jz mErr12DivideByZero
cdq                           ; sign extend eax (W, old T) into rax
mov rdi, WQ                   ; save old T in rdi (use qword to prep for idiv)
mov W, T                      ; prepare dividend (old 2nd) in rax
cdq                           ; sign extend old 2nd into rax
idiv rdi                      ; signed divide 2nd/T (rax:quot, rdx:rem)
mov T, W                      ; new T is quotient from eax
.end:
ret

mMod:                         ; MOD   ( 2nd T -- <remainder 2nd/T> )
fDo PopW, .end
test W, W                     ; make sure divisor is not 0
jz mErr12DivideByZero
cdq                           ; sign extend eax (W, old T) into rax
mov rdi, WQ                   ; save old T in rdi (use qword to prep for idiv)
mov W, T                      ; prepare dividend (old 2nd) in rax
cdq                           ; sign extend old 2nd into rax
idiv rdi                      ; signed divide 2nd/T (rax:quot, rdx:rem)
mov T, edx                    ; new T is remainder from edx
.end:
ret

mDivMod:                      ; /MOD   for 2nd/T: ( 2nd T -- rem quot )
movq rcx, DSDeep              ; make sure there are 2 items on the stack
cmp cl, 2
jb mErr1Underflow
test T, T                     ; make sure divisor is not 0
jz mErr12DivideByZero
sub ecx, 2                    ; fetch old 2nd as dividend to eax (W)
mov W, [DSBase+4*ecx]
cdq                           ; sign extend old 2nd in eax to rax
idiv T                        ; signed divide 2nd/T (rax:quot, rdx:rem)
mov edi, eax                  ; save quotient before address calculation
mov esi, edx                  ; save remainder before address calculation
mov [DSBase+4*ecx], esi       ; remainder goes in second item
mov T, edi                    ; quotient goes in top
ret

mMax:                         ; MAX   ( 2nd T -- the_bigger_one )
fDo PopW, .end
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovl T, W                    ; if old 2nd was less, then use old T for new T
.end:
ret

mMin:                         ; MIN   ( 2nd T -- the_smaller_one )
fDo PopW, .end
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovg T, W                    ; if old 2nd was more, then use old T for new T
.end:
ret

mAbs:                         ; ABS -- Replace T with absolute value of T
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
mov W, T
neg W                         ; check if negated value of old T is positive
test W, W
cmovns T, W                   ; if so, set new T to negated old T
ret

mOnePlus:                     ; 1+ -- Add 1 to T
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
inc T
ret

mTwoPlus:                     ; 2+ -- Add 2 to T
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
add T, 2
ret

mFourPlus:                    ; 4+ -- Add 2 to T
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
add T, 4
ret
