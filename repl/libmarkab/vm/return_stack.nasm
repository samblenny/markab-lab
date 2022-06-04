; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth return stack words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"

extern mDrop
extern Mem
extern mErr19BadAddress
extern mErr20ReturnUnderflow
extern mErr21ReturnFull
extern mPush
extern RSBase

global mI
global mToR
global mRFrom
global mClearReturn
global mJump
global mCall
global mReturn
global mRPopW


mRPushW:                      ; Push W to return stack
movq rdi, RSDeep
cmp dil, RSMax
jnb mErr21ReturnFull
dec edi                       ; calculate store index (subtract 1 for R)
mov [RSBase+4*edi], R         ; push old R to return stack
mov R, W                      ; new R = W
add edi, 2                    ; calculate new stack depth
movq RSDeep, rdi              ; update stack depth
ret

mRPopW:                       ; RPOP - Pop from return stack to W
movq rdi, RSDeep
cmp dil, 1
jb mErr20ReturnUnderflow
dec rdi                       ; calculate new return stack depth
movq RSDeep, rdi
mov W, R                      ; W = old R
dec rdi                       ; calculate fetch index (old depth - 2)
mov R, [RSBase+4*edi]         ; new R = pop item of the return stack
ret

mI:                           ; I -- push loop counter to data stack
movq rdi, RSDeep              ; make sure return stack isn't empty
cmp dil, 1
jb mErr20ReturnUnderflow
mov W, R                      ; push copy of R to data stack
jmp mPush

mToR:                         ; >R -- Move T to R
call mDrop                    ; copy T to W
test VMFlags, VMErr           ; check if it worked
jz mRPushW                    ; if so: push W (old T) to return stack
ret

mRFrom:                       ; R> -- Move R to T
call mRPopW                   ; copy R to W
test VMFlags, VMErr           ; check if it worked
jz mPush                      ; if so: push W (old R) to data stack
ret

; mReset --> see ./data_stack.nasm

mJump:                        ; Jump -- set the VM token instruction pointer
movzx edi, word [Mem+ebp]     ; read pointer literal address from token stream
mov ebp, edi                  ; set I (ebp) to the jump address
ret

mCall:                        ; Call -- make a VM token call
movzx edi, word [Mem+ebp]     ; read pointer literal address from token stream
add ebp, 2                    ; advance I (ebp) past the address literal
push rdi                      ; save the call address
mov W, ebp                    ; push I (ebp) to return stack
call mRPushW
pop rdi                       ; retrieve the call address
mov ebp, edi                  ; set I (ebp) to the call address
ret

mReturn:                      ; Return from end of word
movq rdi, RSDeep
test rdi, rdi                 ; in case of empty return stack, set VMReturn
jz .doneFinal
call mRPopW                   ; pop the return address (should be in CodeMem)
test VMFlags, VMErr
jnz .doneErr
cmp W, Heap                   ; check that: Heap <= W < HeapEnd
jna mErr19BadAddress
cmp W, HeapEnd
jnb mErr19BadAddress          ; if target address is not valid: stop
.done:
mov ebp, W                    ; else: set token pointer to return address
ret
.doneFinal:                   ; set VMReturn flag marking end of outermost word
or VMFlags, VMReturn
ret
.doneErr:
ret
