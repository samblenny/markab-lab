; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth string words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"
%include "libmarkab/generated_macros.nasm"

extern mByteFetch
extern mCompileU16
extern mCompileU8
extern mDrop
extern mDup
extern Mem
extern mErr15HeapFull
extern mErr23BadBufferPointer
extern mErr4NoQuote
extern mOnePlus
extern mPush
extern mRPopW
extern mStrPut.RdiRsi
extern mStrPut.W
extern mToR
extern mWordFetch

global mDotQuoteC
global mDotQuoteI
global mSpace
global mCR
global mEmit
global mEmit.W
global mPrintDPStr


mDotQuoteC:                   ; Print string literal to stdout (token compiled)
movzx ecx, word [Mem+ebp]     ; get length of string in bytes (to adjust I)
add ecx, 2                    ;   add 2 for length word
lea W, [Mem+ebp]              ; I (ebp) should be pointing to {length, chars}
add ebp, ecx                  ; adjust I past string
jmp mStrPut.W

; Print a string literal from the input stream to stdout (interpret mode)
; input bytes come from [IBPtr] using [IBLen] and [IN]
mDotQuoteI:
movzx edi, word [Mem+IBPtr]  ; Fetch input buffer pointer (index to Mem)
cmp edi, BuffersStart        ; Stop if buffer pointer is out of range
jb .doneErr
cmp edi, BuffersEnd
jnb .doneErr
lea edi, [Mem+edi]           ; Resolve buffer pointer to address
movzx esi, word [Mem+IBLen]  ; Load and range check buffer's available bytes
cmp esi, _1KB
jnb .doneErr
movzx ecx, word [Mem+IN]     ; ecx = IN (index to next available byte of TIB)
cmp esi, ecx                 ; stop looking if TIB_LEN <= IN (ran out of bytes)
jna mErr4NoQuote
;------------------------
.forScanQuote:           ; Find a double quote (") character
mov WB, [rdi+rcx]        ; check if current byte is '"'
cmp WB, '"'
jz .finish               ; if so, stop looking
inc ecx                  ; otherwise, advance the index
cmp esi, ecx             ; loop if there are more bytes
jnz .forScanQuote
jmp mErr4NoQuote         ; reaching end of TIB without a quote is an error
;------------------------
.finish:
movzx W, word [Mem+IN]
mov esi, ecx             ; ecx-[IN] is count of bytes copied to Pad
sub esi, W
add edi, W               ; edi = [TIB] + (ecx-[IN]) (old edi was [TIB])
inc ecx                  ; store new IN (skip string and closing '"')
mov word [Mem+IN], cx
;------------------------
test VMFlags, VMCompile  ; check if compiled version if compile mode active
jz mStrPut.RdiRsi        ; nope: do mStrPut.RdiRsi(rdi: *buf, rsi: count)
;------------------------
.compileMode:
cmp esi, 0               ; stop now if string is empty (optimize it out)
jle .end
sub edi, Mem             ; convert edi back to virtual address space
fPush edi,        .end   ; -> {T: *src}                 (string source pointer)
fPush esi,        .end   ; -> {S: *src, T: count}   (loop count: string length)
fPush tDotQuoteC, .end   ; -> {*src, S: count, T: tDotQuoteC}
fDo   CompileU8,  .end   ; -> {S: *src, T: count}      (compile DotQuote token)
fDo   Dup,        .end   ; -> {*src, S: count, T: count}
fDo   CompileU16, .end   ; -> {S: *src, T: count}       (compile string length)
;------------------------
fDo   ToR,        .end   ; -> {T: *src}, {R: count}      (move loop count to R)
.forCopy:
fDo   Dup,        .end   ; -> {S: *src, T: *src}, {R: count}
fDo   ByteFetch,  .end   ; -> {S: *src, T: [*src]}, {R: count}   (get src byte)
fDo   CompileU8,  .end   ; -> {T: *src}, {R: count}          (compile src byte)
fDo   OnePlus,    .end   ; -> {T: *src++}, {R: count}    (increment source ptr)
dec R                    ; -> {T: *src}, {R: count--}    (decrement loop count)
jnz .forCopy             ; keep looping if R is non-zero
fDo   Drop,       .end   ; -> {}, {R: count)        (discard source ptr from T)
fDo   RPopW,      .end   ; -> {}, {}                (discard loop count from R)
;------------------------
.end:
ret
;------------------------
.doneErr:
jmp mErr23BadBufferPointer

mSpace:                       ; Print a space to stdout
mov W, ' '
jmp mEmit.W

mCR:                          ; Print a CR (newline) to stdout
mov W, 10
jmp mEmit.W

mEmit:                        ; Print low byte of T as ascii char to stdout
mov W, T
push WQ
call mDrop
pop WQ
.W:                           ; Print low byte of W as ascii char to stdout
movzx esi, WB                 ; store WB in [edi: EmitBuf]
lea edi, [Mem+EmitBuf]
mov [edi], esi
xor esi, esi                  ; esi = 1 (count of bytes in *edi)
inc esi
jmp mStrPut.RdiRsi

mPrintDPStr:             ; Print string from [DP]
push rbp
fPush DP,        .end    ; -> {T: DP (pointer to end of dictionary)}
fDo   WordFetch, .end    ; -> {T: address (end of dictionary)}
fDo   Dup,       .end    ; -> {S: addr, T: addr}
fDo   ByteFetch, .end    ; -> {S: addr, T: string length count}
mov ebp, T               ; ebp: count
fDo   Drop,      .end    ; -> {T: addr}
fDo   OnePlus,   .end    ; -> {T: addr+1}
lea edi, [Mem+T]         ; edi: *buf
mov esi, ebp             ; esi: count
call mStrPut.RdiRsi      ; print the string at [DP]
fDo   Drop,      .end    ; drop  -> {}
.end:
pop rbp
ret
