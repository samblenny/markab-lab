; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth data stack words (meant to be included in ../libmarkab.nasm)

mNop:                         ; NOP - do nothing
ret

mDup:                         ; DUP - Push T
movq rdi, DSDeep              ; check if stack is empty
test rdi, rdi
jz mErr1Underflow
mov W, T
jmp mPush

mSwap:                        ; SWAP - Swap T and second item on stack
movq rdi, DSDeep              ; check if stack depth is >= 2
cmp dil, 2
jb mErr1Underflow
sub edi, 2
xchg T, [DSBase+4*edi]
ret

mOver:                        ; OVER - Push second item on stack
movq rdi, DSDeep              ; check if stack depth is >= 2
cmp dil, 2
jb mErr1Underflow
sub edi, 2
mov W, [DSBase+4*edi]
jmp mPush

mClearStack:                  ; CLEARSTACK - Drop all stack cells
xor rdi, rdi
movq DSDeep, rdi
ret

mPush:                        ; PUSH - Push W to data stack
movq rdi, DSDeep
cmp dil, DSMax
jnb mErr2Overflow
dec edi                       ; calculate store index of old_depth-2+1
mov [DSBase+4*edi], T         ; store old value of T
mov T, W                      ; set T to caller's value of W
add edi, 2                    ; CAUTION! `add di, 2` or `dil, 2` _not_ okay!
movq DSDeep, rdi              ; this depth includes T + (DSMax-1) memory items
ret

mPopW:                        ; POP  - alias for mDrop (which copies T to W)
mDrop:                        ; DROP - pop T, saving a copy in W
movq rdi, DSDeep              ; check if stack depth >= 1
cmp dil, 1
jb mErr1Underflow
dec rdi                       ; new_depth = old_depth-1
movq DSDeep, rdi
mov W, T
dec rdi                       ; convert depth to second item index (old_depth-2)
mov T, [DSBase+4*edi]
ret

mU8:                          ; Push zero-extended unsigned 8-bit literal
movzx W, byte [Mem+ebp]       ; read literal from token stream
inc ebp                       ; ajust I
jmp mPush

mI8:                          ; Push sign-extended signed 8-bit literal
movsx W, byte [Mem+ebp]       ; read literal from token stream
inc ebp                       ; ajust I
jmp mPush

mU16:                         ; Push zero-extended unsigned 16-bit literal
movzx W, word [Mem+ebp]       ; read literal from token stream
add ebp, 2                    ; adjust I
jmp mPush

mI16:                         ; Push sign-extended signed 16-bit literal
movsx W, word [Mem+ebp]       ; read literal from token stream
add ebp, 2                    ; adjust I
jmp mPush

mI32:                         ; Push signed 32-bit dword literal
mov W, dword [Mem+ebp]        ; read literal from token stream
add ebp, 4                    ; adjust I
jmp mPush
