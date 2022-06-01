; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth error words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"

extern datErr10en
extern datErr11ntl
extern datErr12dbz
extern datErr13aor
extern datErr14bwt
extern datErr15hf
extern datErr16stl
extern datErr17snc
extern datErr18esc
extern datErr19ba
extern datErr1se
extern datErr20rsu
extern datErr21rsf
extern datErr22ltl
extern datErr23bbp
extern datErr24cvt
extern datErr25bor
extern datErr26fin
extern datErr27bfi
extern datErr28dpo
extern datErr29bvl
extern datErr2sf
extern datErr30cow
extern datErr3bt
extern datErr4nq
extern datErr5nf
extern datErr6of
extern datErr8np
extern datErr9df
extern mClearReturn
extern mClearStack
extern mDot.W
extern mStrPut.RdiRsi
extern mStrPut.W

global mErr1Underflow
global mErr2Overflow
global mErr3BadToken
global mErr4NoQuote
global mErr5NumberFormat
global mErr6Overflow
global mErr8NoParen
global mErr9DictFull
global mErr10ExpectedName
global mErr11NameTooLong
global mErr12DivideByZero
global mErr13AddressOOR
global mErr14BadWordType
global mErr15HeapFull
global mErr16ScreenTooLong
global mErr17SemiColon
global mErr18ExpectedSemiColon
global mErr19BadAddress
global mErr20ReturnUnderflow
global mErr21ReturnFull
global mErr22LoopTooLong
global mErr23BadBufferPointer
global mErr24CoreVocabTooLong
global mErr25BaseOutOfRange
global mErr26FormatInsert
global mErr27BadFormatIndex
global mErr28DPOutOfRange
global mErr29BadVocabLink
global mErr30CompileOnlyWord


mErrPutW:                     ; Print error from W and set error flag
call mStrPut.W
or VMFlags, VMErr
ret

mErr:
or VMFlags, VMErr             ; set error condition flag (hide OK prompt)
ret

mErr1Underflow:               ; Error 1: Stack underflow
lea W, [datErr1se]
jmp mErrPutW

mErr2Overflow:                ; Error 2: Stack overflow
call mClearStack              ; clear the stack
lea W, [datErr2sf]            ; print error message
jmp mErrPutW

mErr3BadToken:                ; Error 3: Bad token (rbp: instruction pointer)
lea W, [datErr3bt]            ; print error message
call mStrPut.W
movzx W, byte [rbp]           ; load token value
call mDot.W                   ; print token value
mov W, ebp                    ; print token's instruction pointer
call mDot.W
jmp mErr

mErr4NoQuote:                 ; Error 4: unterminated quoted string
lea W, [datErr4nq]            ; print error message
jmp mErrPutW

mErr5NumberFormat:            ; Error 5: Number format
lea W, [datErr5nf]
jmp mErrPutW

mErr6Overflow:                ; Error 6: overflow, args{rdi: *buf, rsi: count}
push rdi
push rsi
lea W, [datErr6of]            ; print error message
call mStrPut.W
pop rsi
pop rdi
call mStrPut.RdiRsi           ; print word that caused the problem
or VMFlags, VMErr
ret

mErr8NoParen:                 ; Error 8: comment had '(' without matching ')'
lea W, [datErr8np]
jmp mErrPutW

mErr9DictFull:                ; Error 9: Dictionary is full
lea W, [datErr9df]
jmp mErrPutW

mErr10ExpectedName:           ; Error 10: Expected a name
lea W, [datErr10en]
jmp mErrPutW

mErr11NameTooLong:            ; Error 11: Name too long
lea W, [datErr11ntl]
jmp mErrPutW

mErr12DivideByZero:           ; Error 12: Divide by zero
lea W, [datErr12dbz]
jmp mErrPutW

mErr13AddressOOR:             ; Error 13: Address out of range
lea W, [datErr13aor]
jmp mErrPutW

mErr14BadWordType:            ; Error 14: Bad word type
lea W, [datErr14bwt]
jmp mErrPutW

mErr15HeapFull:               ; Error 15: Heap full
lea W, [datErr15hf]
jmp mErrPutW

mErr16ScreenTooLong:          ; Error 16: Screen too long
lea W, [datErr16stl]
jmp mErrPutW

mErr17SemiColon:              ; Error 17: ; when not compiling
lea W, [datErr17snc]
jmp mErrPutW

mErr18ExpectedSemiColon:      ; Error 18: Expected ;
lea W, [datErr18esc]
jmp mErrPutW

mErr19BadAddress:             ; Error 19: Bad address
lea W, [datErr19ba]
jmp mErrPutW

mErr20ReturnUnderflow:        ; Error 20: Return stack underflow
lea W, [datErr20rsu]
jmp mErrPutW

mErr21ReturnFull:             ; Error 21: Return stack full
call mClearReturn             ; clear return stack
lea W, [datErr21rsf]
jmp mErrPutW

mErr22LoopTooLong:            ; Error 22: Loop too long
lea W, [datErr22ltl]
jmp mErrPutW

mErr23BadBufferPointer:       ; Error 23: Bad buffer pointer
lea W, [datErr23bbp]
jmp mErrPutW

mErr24CoreVocabTooLong:       ; Error 24: Bad buffer pointer
lea W, [datErr24cvt]
jmp mErrPutW

mErr25BaseOutOfRange:         ; Error 25: Base out of range
lea W, [datErr25bor]
jmp mErrPutW

mErr26FormatInsert:           ; Error 26: Format insert
lea W, [datErr26fin]
jmp mErrPutW

mErr27BadFormatIndex:         ; Error 27: Bad format index
lea W, [datErr27bfi]
jmp mErrPutW

mErr28DPOutOfRange:           ; Error 28: DP out of range
lea W, [datErr28dpo]
jmp mErrPutW

mErr29BadVocabLink:           ; Error 29: Bad vocab link
lea W, [datErr29bvl]
jmp mErrPutW

mErr30CompileOnlyWord:        ; Error 30: Compile-only word
lea W, [datErr30cow]
jmp mErrPutW
