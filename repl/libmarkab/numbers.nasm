; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth number words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"

extern mDrop
extern Mem
extern mErr1Underflow
extern mErr25BaseOutOfRange
extern mErr26FormatInsert
extern mErr27BadFormatIndex
extern mErr5NumberFormat
extern mStrPut.RdiRsi

global mHex
global mDecimal
global mDot
global mDot.W
global mFmtRtlClear
global mFmtRtlInt32
global mFmtRtlSpace
global mFmtRtlPut


mHex:                         ; Set number base to 16
mov word [Mem+Base], word 16
ret

mDecimal:                     ; Set number base to 10
mov word [Mem+Base], word 10
ret

mDot:                         ; Print T using number base
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
mov W, T
push WQ
call mDrop
pop WQ
.W:                           ; Print W using number base
call mFmtRtlClear             ; Clear the buffer
mov rdi, WQ
call mFmtRtlInt32             ; format W
call mFmtRtlSpace             ; add a space
jmp mFmtRtlPut                ; print number formatting buffer

mFmtRtlClear:                 ; Clear number formatting buffer
mov word [Mem+FmtLI], FmtEnd  ; leftmost available byte = rightmost byte
ret

mFmtRtlSpace:                 ; Insert (rtl) a space into the Fmt buffer
xor rdi, rdi
mov dil, ' '
jmp mFmtRtlInsert

mFmtRtlMinus:                 ; Insert (rtl) a '-' into the Fmt buffer
xor rdi, rdi
mov dil, '-'
jmp mFmtRtlInsert

mFmtRtlInsert:                ; Insert (rtl) byte from rdi into Fmt buffer
movzx esi, word [Mem+FmtLI]   ; load index of leftmost available byte
cmp esi, Fmt                  ; stop if buffer is full
jb mErr26FormatInsert
mov byte [Mem+rsi], dil       ; store whatever was in low byte of rdi
dec esi                       ; dec esi to get next available byte
mov word [Mem+FmtLI], si      ; store the new leftmost byte index
ret

mFmtRtlPut:                   ; Print the contents of Fmt
movzx ecx, word [Mem+FmtLI]   ; index to leftmost available byte of Fmt
inc ecx                       ; convert from available byte to filled byte
cmp ecx, Fmt                  ; check that index is within Fmt buffer bounds
jb mErr27BadFormatIndex
cmp ecx, FmtEnd
ja mErr27BadFormatIndex
lea rdi, [Mem+ecx]            ; edi: pointer to left end of format string
mov esi, FmtEnd+1             ; esi: count of bytes in format string
sub esi, ecx
jmp mStrPut.RdiRsi            ; mStrPut.RdiRsi(rdi: *buf, rsi: count)

; Format edi (int32), right aligned in Fmt, according to current number base.
;
; This formats a number from edi into Fmt, using an algorithm that puts
; digits into the buffer moving from right to left. The idea is to match the
; order of divisions, which necessarily produces digits in least-significant to
; most-significant order. This is intended for building a string from several
; numbers in a row (like with .S).
;
; My intent with keeping this mechanism entirely separate from the stack was to
; have a debugging capability that still works if the stack is full. Probably
; there is a better way to handle this. The current approach feels too complex
; and is a big hassle to maintain.
;
; TODO: maybe port this from assembly to Forth code that loads at runtime?
;
mFmtRtlInt32:                 ; Format edi (int32) using current number Base
push rbp
mov dword [Mem+FmtQuo], edi   ; Save edi as initial Quotient because the digit
;...                          ;  divider takes its dividend from the previous
;...                          ;  digit's quotient (stored in [FmtQuo])
call mFmtFixupQuoSign         ; Prepare for decimal i32 (signed) or hex u32
;...                          ;  (unsigned) depending on current [Base]
mov ebp, 11                   ; ebp: loop limit (11 digits is enough for i32)
;-----------------------------
.forDigits:
call mFmtDivideDigit          ; Divide out the least significant digit
test VMFlags, VMErr           ;  stop if there was an error
jnz .doneErr
call mFmtRemToASCII           ; Format devision remainder (digit) into Fmt buf
test VMFlags, VMErr           ;  stop if there was an error
jnz .doneErr
dec ebp                       ; stop if loop has been running too long
jz .doneErrOverflow
mov W, dword [Mem+FmtQuo]
test W, W                     ; loop unil quotient is zero (normal exit path)
jnz .forDigits
;-----------------------------
.done:                        ; Normal exit
pop rbp
mov al, byte [Mem+FmtSgn]     ; load sign flag for the number being formatted
test al, al                   ; check if sign is negative
setnz cl
movzx eax, word [Mem+Base]    ; check if base is decimal
cmp ax, 10
sete dl
test cl, dl
jnz mFmtRtlMinus              ; negative and base 10 means add a '-'
ret
;-----------------------------
.doneErr:                     ; This exit path is unlikely unles there's a bug
pop rbp
ret
;-----------------------------
.doneErrOverflow:             ; This exit path is also unlikely, unless a bug
pop rbp
jmp mErr5NumberFormat


mFmtFixupQuoSign:             ; Fixup state vars for decimal i32 or hex u32
mov edi, dword [Mem+FmtQuo]   ; load initial quotent (number to be formatted)
test edi, edi                 ; check if sign is negative
sets cl
movzx eax, word [Mem+Base]    ; check if base is decimal
cmp ax, 10
sete dl
test cl, dl                   ; if negative and decimal, prepare for i32
jnz .prepForNegativeI32
;-------------------
.prepForU32:                  ; Do this for positive decimal or any hex
mov byte [Mem+FmtSgn], 0      ;  sign is positive, do not take absolute value
ret
;-------------------
.prepForNegativeI32:          ; Do this for negative decimal
mov byte [Mem+FmtSgn], -1     ;  save negative sign in [FmtSign]
neg edi                       ;  take absolute value of the initial quotient
mov dword [Mem+FmtQuo], edi
ret


mFmtDivideDigit:              ; Do the division to format one digit of an i32
mov eax, dword [Mem+FmtQuo]   ; idiv dividend (old quotient) goes in [rdx:rax]
xor edx, edx                  ;  (we're using i32, not i64, so zero rdx)
movzx ecx, word [Mem+Base]    ; idiv divisor (number base) goes in rcx
xor edi, edi
mov dil, 2                    ; limit Base to range 2..16 to avoid idiv trouble
cmp ecx, edi
jl mErr25BaseOutOfRange
mov dil, 16
cmp ecx, edi
jg mErr25BaseOutOfRange
;-------
idiv rcx                      ; idiv: {[rdx:rax]: dividend, operand: divisor}
;-------                              result {rax: quotient, rdx: remainder}
mov dword [Mem+FmtQuo], eax   ; idiv puts quotient in rax
mov dword [Mem+FmtRem], edx   ; idiv puts remainder in rdx
ret

mFmtRemToASCII:               ; Format division remainder as ASCII digit
mov edi, dword [Mem+FmtRem]   ; remainder should be in range 0..([Base]-1)
movzx rcx, dil                ; prepare digit as n-10+'A' (in case n in 10..15)
add cl, ('A'-10)
add dil, '0'                  ; prepare digit as n+'0' (in case n in 0..9)
cmp dil, '9'                  ; rdx = pick the right digit (hex or decimal)
cmova rdi, rcx
jmp mFmtRtlInsert             ; insert low byte of rdi into Fmt buffer
