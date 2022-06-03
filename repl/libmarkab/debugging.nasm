; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth debugging words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"

extern datCallDP
extern datContext
extern datDotS
extern datDotSNone
extern datDP
extern datDPStr
extern datForthP
extern datHeap
extern datHeapEnd
extern datLast
extern datVoc0Head
extern DSBase
extern mCR
extern mDot
extern mDup
extern mFmtRtlClear
extern mFmtRtlInt32
extern mFmtRtlPut
extern mFmtRtlSpace
extern mPrintDPStr
extern mPush
extern mStrPut.W
extern mWordFetch
extern RSBase
extern Voc0Head

global mDotS
global mDotRet
global mDumpVars


; Nondestructively print stack in current number base.
;
; The indexing math is tricky. The stack depth (DSDeep) tracks total cells on
; the stack, including T, which is a register. So, the number of stack cells in
; memory is DSDeep-1. Some examples:
;
;   DSDeep   Top  Second_cell  Third_cell  Fourth_cell
;        0    --           --          --           --
;        1     T           --          --           --
;        2     T   [DSBase+0]          --           --
;        3     T   [DSBase+4]  [DSBase+0]           --
;        4     T   [DSBase+8]  [DSBase+4]   [DSBase+0]
;
mDotS:
push rbp
movq rdi, DSDeep
test rdi, rdi         ; if stack is empty, print the empty message
jz .doneEmpty
call mFmtRtlClear     ; otherwise, prepare the formatting buffer
;---------------------
xor ebp, ebp          ; start index at 1 because of T
inc ebp
mov edi, T            ; prepare for mFmtRtlInt32(edi: T)
;---------------------
.for:                 ; Format stack cells
call mFmtRtlInt32     ; format(edi: current stack cell) into PadRtl
test VMFlags, VMErr
jnz .doneFmtErr
call mFmtRtlSpace     ; add a space (rtl, so space goes to left of number)
inc ebp               ; inc index
movq WQ, DSDeep       ; stop if all stack cells have been formatted
cmp ebp, W
ja .done
sub W, ebp            ; otherwise, prepare for mFmtRtlInt32(rdi: stack cell)
mov edi, [DSBase+4*W]
jmp .for              ; keep looping
;---------------------
.done:
pop rbp
call mFmtRtlSpace     ; add a space (remember this is right to left)
call mFmtRtlPut       ; print the format buffer
ret
;---------------------
.doneEmpty:
pop rbp
lea W, [datDotSNone]  ; print empty stack message
jmp mStrPut.W
;---------------------
.doneFmtErr:
pop rbp
ret


mDotRet:              ; Nondestructively print return stack in current base
push rbp
movq rdi, RSDeep
test rdi, rdi         ; if stack is empty, print the empty message
jz .doneEmpty
call mFmtRtlClear     ; otherwise, prepare the formatting buffer
;---------------------
xor ebp, ebp          ; start index at 1 because of R
inc ebp
mov edi, R            ; prepare for mFmtRtlInt32(edi: R)
;---------------------
.for:                 ; Format stack cells
call mFmtRtlInt32     ; format(edi: current stack cell) into PadRtl
call mFmtRtlSpace     ; add a space (rtl, so space goes to left of number)
inc ebp               ; inc index
movq WQ, RSDeep       ; stop if all stack cells have been formatted
cmp ebp, W
ja .done
sub W, ebp            ; otherwise, prepare for mFmtRtlInt32(rdi: stack cell)
mov edi, [RSBase+4*W]
jmp .for              ; keep looping
;---------------------
.done:
pop rbp
call mFmtRtlSpace     ; add a space (remember this is right to left)
call mFmtRtlPut       ; print the format buffer
ret
;---------------------
.doneEmpty:
pop rbp
lea W, [datDotSNone]  ; print empty stack message
jmp mStrPut.W

mDumpVars:               ; Debug dump dictionary variables and stack
call mCR
fPush [Voc0Head], .end1  ; [Voc0Head]   <address> <contents>
lea W, [datVoc0Head]
call   .mDumpOneVar
fPush ForthP,     .end1  ; ForthP       <address> <contents>
lea W, [datForthP]
call   .mDumpOneVar
fPush Context,    .end1  ; Context      <address> <contents>
lea W, [datContext]
call   .mDumpOneVar
fPush DP,         .end1  ; DP           <address> <contents>
lea W, [datDP]
call   .mDumpOneVar
fPush Last,       .end1  ; Last         <address> <contents>
lea W, [datLast]
call   .mDumpOneVar
fPush CallDP,  .end1     ; CallDP       <address> <contents>
lea W, [datCallDP]
call   .mDumpOneVar
fPush Heap,       .end1  ; Heap         <address> <contents>
lea W, [datHeap]
call   .mDumpLabelW
fPush HeapEnd,    .end1  ; HeapEnd      <address> <contents>
lea W, [datHeapEnd]
call   .mDumpLabelW
lea W, [datDPStr]        ; string([DP]) <string>
call mStrPut.W
call mPrintDPStr
call mCR
lea W, [datDotS]         ; .s           <stack-items>
call mStrPut.W
call mDotS
call mCR
.end1:
ret
.mDumpOneVar:            ; Print a line (caller prepares W and T)
call mStrPut.W           ; Print name of string (W points to string)
fDo   Dup,       .end2   ; copy {T: address} -> {S: addr, T: addr}
fDo   Dot,       .end2   ; print -> {T: addr}
fDo   WordFetch, .end2   ; fetch -> {T: contents of addr}
fDo   Dot,       .end2   ; print -> {}
.end2:
call mCR
ret
.mDumpLabelW:            ; This is for values that aren't pointers
call mStrPut.W
call mDot
call mCR
ret
