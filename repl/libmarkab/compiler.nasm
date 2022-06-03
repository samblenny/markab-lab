; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth compiler words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"
%include "libmarkab/generated_macros.nasm"

extern mByteStore
extern mDrop
extern mDup
extern mEmit.W
extern Mem
extern mErr5NumberFormat
extern mErr6Overflow
extern mErr9DictFull
extern mErr10ExpectedName
extern mErr11NameTooLong
extern mErr15HeapFull
extern mErr17SemiColon
extern mErr18ExpectedSemiColon
extern mErr19BadAddress
extern mErr20ReturnUnderflow
extern mErr30CompileOnlyWord
extern mEqual
extern mFourPlus
extern mLess
extern mLessEq
extern mOnePlus
extern mPopW
extern mPlus
extern mPush
extern mRPopW
extern mStore
extern mSwap
extern mTwoPlus
extern mWordFetch
extern mWordStore

global mColon
global mSemiColon
global mCreate
global mWord
global mAllot
global mLowercase
global mNumber
global mCompileLiteral
global mCompileU8
global mCompileU16
global mCompileU32
global mHere
global mLast
global mIf
global mElse
global mEndIf
global mFor
global mNext


mColon:                    ; COLON - define a word
test VMFlags, VMCompile    ; if already in compile mode, invoking : is an error
jnz mErr18ExpectedSemiColon
fDo   Here,       .end     ; -> {T: [DP]}     (save [DP] for possible rollback)
fPush ColonDP,    .end     ; -> {S: [DP], ColonDP}
fDo   WordStore,  .end     ; -> {}
fPush Last,       .end     ; -> {T: Last}   (save [Last] for possible rollback)
fDo   WordFetch,  .end     ; -> {T: [Last]}
fPush ColonLast,  .end     ; -> {S: [Last], T: ColonLast}
fDo   WordStore,  .end     ; -> {}
;-------------------------
fDo  Create,      .rollbk  ; add name from input stream to dictionary
;-------------------------
fPush TpCode,     .rollbk  ; -> {T: TpCode}
fDo   CompileU8,  .rollbk  ; -> {}                     (compile .type = TpCode)
fPush ColonDP,    .rollbk  ; -> {T: ColonDP}
fDo   WordFetch,  .rollbk  ; -> {T: [DP] (start of def, before create)}
fPush Last,       .rollbk  ; -> {S: [DP] (start of def) T: Last}
fDo   WordStore,  .rollbk  ; -> {}              (commit new word to dictionary)
fDo   Here,       .rollbk  ; -> {T: [DP] (after name, for use by `;`)
or VMFlags, VMCompile      ; set compile mode
ret
;-------------------------
.rollbk:                   ; revert changes to [DP] and [Last]
fPush ColonLast,   .end    ; get old value of [Last]
fDo   WordFetch,   .end
fPush Last,        .end
fDo   WordStore,   .end    ; set [Last] to the old [Last]
fPush ColonDP,     .end    ; get old value of [DP]
fDo   WordFetch,   .end
fPush DP,          .end
fDo   WordStore,   .end    ; set [DP] to the old [DP]
ret
;-------------------------
.end:                      ; Last resort exit path (DP or Last may be broken)
ret


mSemiColon:                ; SEMICOLON - end definition of a new word
test VMFlags, VMCompile    ; if not in compile mode, invoking ; is an error
jz mErr17SemiColon
;-------------------------
.optimizerCheck:           ; Check if tail call optimization is possible
;...                       ;  (requires that the last call token occurred
;...                       ;  within the current definition, not before)
fPush CallDP,     .end     ; -> {S: [DP] (old, from `:`), T: CallDP}
fDo   WordFetch,  .end     ; -> {S: [DP] (:), T: [CallDP]}
fDo   LessEq,     .end     ; -> {T: ((S<=T): -1 or (S>T): 0)}
fDo   PopW,       .end     ; -> {}, {W: true(-1) or false(0)} 
test W, W
jz .normalReturn           ; skip optimization if last call not in this def
fDo   Here,       .end     ; Calculate where tCall would be if there was a call
sub T, 3                   ;  right before this semicolon
fPush CallDP,     .end     ; Compare that calculation to the last call address
fDo   WordFetch,  .end
fDo   Equal,      .end     ; -> {T: (equal: -1 or not-equal: 0)}
fDo   PopW,       .end     ; -> {}, {W: true(-1) or false(0)}
test W, W
jz .normalReturn           ; skip optimization if last word of def was not call
;-------------------------
.rewriteTailCall:          ; Do a tail call optimization
fPush CallDP,     .end     ; -> {T: CallDP}
fDo   WordFetch,  .end     ; -> {T: [CallDP]}
fPush tJump,      .end     ; -> {S: [CallDP], T: tJump}
fDo   Swap,       .end     ; -> {S: tJump, T: [CallDP]}
fDo   ByteStore,  .end     ; -> {}                (change tCall token to tJump)
;...                       ; continue to .normalReturn
;-------------------------
.normalReturn:             ; Compile a Return token
fPush tReturn,    .end     ; -> {T: tReturn}
fDo   CompileU8,  .end     ; -> {}
and VMFlags, (~VMCompile)  ; clear compile mode flag
ret
;-------------------------
.end:                      ; Something went wrong, so clear compile flag
and VMFlags, (~VMCompile)
ret


; CREATE - Add a name to the dictionary
; struct format: {dd .link, db .nameLen, <name>, db .wordType, dw .param}
mCreate:
movzx esi, word [Mem+DP]     ; load dictionary pointer [DP]
cmp esi, HeapEnd-HReserve    ; check if dictionary has room (link + reserve)
jnb mErr9DictFull
;---------------------------
push rsi                     ; save a copy of [DP] in case rollback needed
movzx edi, word [Mem+Last]   ; append {.link: [Last]} to dictionary
mov word [Mem+esi], di
add esi, 2                   ; update [DP]
mov word [Mem+DP], si
push rsi                     ; store pointer to {.nameLen, <name>}
call mWord                   ; append word from [TIB+IN] as {.nameLen, <name>}
pop rsi                      ; load pointer to {.nameLen, <name>}
test VMFlags, VMErr          ; check for errors
jnz .doneErr
;--------------------------
                         ; Lowercase the name (for case-insensitive lookups)
lea rdi, [Mem+rsi]       ; prepare {rdi, rsi} args for the name that was just
movzx rsi, byte [rdi]    ;   stored at [DP] (its value before mWord)
inc rdi                  ; skip the length byte of {db .nameLen, <name>}
call mLowercase          ; mLowercase(rdi: *buf, rsi: count)
;------------------------
.done:
pop rsi                  ; commit dictionary changes
ret
;------------------------
.doneErr:
pop rsi                  ; roll back dictionary changes
mov word [Mem+DP], si    ; Restore DP to its old value
xor edi, edi             ; Zero the first byte at [DP] so it will behave as a
mov di, si               ;  length-prefixed string that is empty
mov byte [Mem+rdi], 0
ret

; WORD - Copy a word from [TIB+IN] to [DP] in format: {db length, <the-word>}
mWord:
push rbp
push rbx
fPush IBPtr,     .err1   ; push  -> {T: address of pointer to input buffer}
fDo   WordFetch, .err1   ; fetch -> {T: pointer to input buffer}
mov ebp, T               ; ebp: *buf (virtual address)
fDo   Drop,      .err1   ; drop  -> {}
fPush IBLen,     .err1   ; push  -> {T: address of buffer size}
fDo   WordFetch, .err1   ; fetch -> {T: buffer }
mov ebx, T               ; ebx: count
fDo   Drop,      .err1   ; drop  -> {}
fPush IN,        .err1   ; push  -> {T: address of current input index}
fDo   WordFetch, .err1   ; fetch -> {T: input index}
call mPopW               ; pop   -> {}, [W: input index]
cmp W, ebx               ; stop if there are no input bytes (index >= count)
jge .err1
mov edi, ebp             ; edi: *buf  (virtual address in Mem)
mov esi, W               ; esi: [IN]  (index into *buf)
mov ecx, ebx             ; ecx: count (maximum index of *buf)
mov W, edi               ; stop if (*buf + count - 1) is out of range
add W, ecx
dec W
cmp W, MemSize
jge .err1
;////////////////////////
jmp .forScanStart
;------------------------
.isWhitespace:              ; Check if W is whitespace (set r10b if so)
cmp WB, ' '                 ; check for space
sete r10b
cmp WB, 10                  ; check for LF
sete r11b
or r10b, r11b
cmp WB, 13                  ; check for CR
sete r11b
or r10b, r11b               ; Zero flag will be set for non-whitespace
ret
;------------------------
.forScanStart:              ; Scan past whitespace to find word-start boundary
mov WB, byte [Mem+edi+esi]  ; check if current byte is non-space
call .isWhitespace
jz .forScanEnd              ; jump if word-start boundary was found
inc esi                     ; otherwise, advance past the ' '
mov word [Mem+IN], si       ; update IN (save index to start of word)
cmp esi, ecx                ; loop if there are more bytes
jb .forScanStart
jmp .doneNone               ; stop if buffer was all whitespace
;------------------------
.forScanEnd:                ; Scan for word-end boundary (space or buffer end)
mov WB, byte [Mem+edi+esi]
call .isWhitespace
jnz .wordSpace
inc esi
cmp esi, ecx
jb .forScanEnd              ; loop if there are more input bytes
;////////////////////////
                            ; Handle word terminated by end of stream
.wordEndBuf:                ; now: {[IN]: start index, esi: end index}
movzx W, word [Mem+IN]      ; prepare arguments for .copyWordRdiRsi
sub esi, W                  ; convert: index_of_word_end -> word_length
add edi, W                  ; convert: [IBPtr] -> start_of_word_pointer
movzx W, word [Mem+IN]      ; update [IN]
add W, esi
mov word [Mem+IN], WW
jmp .copyWordRdiRsi         ; Copy word to end of dictionary
;------------------------
                            ; Handle word terminated by whitespace
.wordSpace:                 ; now: {[IN]: start index, rsi: end index + 1}
movzx W, word [Mem+IN]      ; prepare word as {rdi: *buf, rsi: count}
sub esi, W                  ; convert: index_of_word_end -> word_length
add edi, W                  ; convert: [IBPtr] -> start_of_word_pointer
add W, esi                  ; update IN to point 1 past the space
inc W
mov word [Mem+IN], WW    ; (then fall through to copy word)
;////////////////////////
.copyWordRdiRsi:           ; Copy word {rdi: *buf, rsi: count} to [Mem+DP]
movzx rcx, word [Mem+DP]   ; load dictionary pointer (DP)
cmp ecx, HeapEnd-255       ; stop if dictionary is full
jnb mErr9DictFull
cmp esi, 255               ; stop if word is too long (max 255 bytes)
ja mErr11NameTooLong
;------------------------
mov byte [Mem+ecx], sil    ; store {.nameLen: <byte count>} in [Mem+[DP]]
inc ecx
xor edx, edx               ; zero source index
.forCopy:
mov WB, [Mem+edi+edx]      ; load [Mem + [IBPtr] + [IN] + edx]
mov byte [Mem+ecx], WB     ; store it to [Mem + [DP]]
inc edx                    ; advance source index (edx)
inc ecx                    ; advance [DP]
cmp rdx, rsi               ; keep looping if source index < length of word
jb .forCopy
;////////////////////////
.done:
pop rbx
pop rbp
mov word [Mem+DP], cx      ; store the new dictionary pointer
ret
;------------------------
.doneNone:                 ; didn't get a word, so clear string count at [DP]
fPush 0,         .err1     ; push  -> {T: 0}
fPush DP,        .err1     ; push  -> {S: 0, T: DP}
fDo   WordFetch, .err1     ; push  -> {S: 0, T: [DP]}
fDo   ByteStore, .err1     ; store -> {}
pop rbx
pop rbp
ret
;------------------------
.err1:
pop rbx
pop rbp
and VMFlags, ~VMCompile    ; clear compile flag
call mErr10ExpectedName
ret

mAllot:                ; ALLOT -- Increase Dictionary Pointer (DP) by T
fDo   Here,      .end  ; -> {S: number, T: [DP] (address of first free byte)}
fDo   Plus,      .end  ; -> {T: [DP]+number}
cmp T, HeapEnd-HReserve
jge mErr15HeapFull     ; stop if requested allocation is too large
fPush DP,        .end  ; -> {S: [DP]+number, T: DP}
fDo   WordStore, .end  ; -> {}
.end:
ret

; Convert string at {edi: *buf, esi: count} to lowercase (modify in place)
mLowercase:
test esi, esi         ; stop if string is empty
jz .done
xor ecx, ecx
.for:
mov WB, [edi+ecx]     ; load [rdi+i] for i in 0..(rsi-1)
cmp WB, 'A'           ; convert WB to lowercase without branching
setae r10b
cmp WB, 'Z'
setbe r11b
test r10b, r11b
setnz r10b            ; ...at this point, r10 is set if dl is in 'A'..'Z'
mov r11b, WB          ; ...speculatively prepare a lowercase (c+32) character
add r11b, 32
test r10b, r10b       ; ...swap in the lowercase char if needed
cmovnz W, r11d
mov [edi+ecx], WB     ; store the lowercased byte
inc ecx
dec esi               ; loop until all bytes have been checked
jnz .for
.done:
ret

mNumber:              ; Parse & push i32 from word (rdi: *buf, rsi: count)
test rsi, rsi              ; check that count > 0
cmp rsi, 0
jle .doneErr1
;------------------------
xor r8, r8                 ; zero index
xor r9, r9                 ; zero ASCII digit
mov ecx, esi               ; rcx = count of bytes in word buffer
;------------------------
movzx W, word [Mem+Base]   ; check that base is 10 or 16
cmp WB, 10
jz .decimal                ; use decimal conversion
cmp WB, 16
jz .hex                    ; use hex conversion
jmp .doneErr2              ; oops... Base is not valid
;/////////////////////
.decimal:             ; Attempt to convert word as signed decimal number
xor WQ, WQ            ; zero accumulator
mov r9b, [rdi]        ; check for leading "-" indicating negative
cmp r9b, '-'
setz r10b             ; r10b: set means negative, clear means positive
jnz .forDigits        ; jump if positive, otherwise continue to .negative
;---------------------
.negative:            ; Skip '-' byte if negative
inc r8                ; advance index
dec rcx               ; decrement loop limit counter
jz .doneNaN
;---------------------
.forDigits:           ; Convert decimal digits
mov r9b, [rdi+r8]     ; get next byte of word (maybe digit, or maybe not)
sub r9b, '0'          ; attempt to convert from ASCII digit to uint8
cmp r9b, 9
jnbe .doneNaN         ; jump to error if result is greater than uint8(9)
imul WQ, 10           ; scale accumulator
add WQ, r9            ; add value of digit to accumulator
jo mErr6Overflow      ; check for 64-bit overflow now (31-bit comes later)
inc r8                ; keep looping until end of word
dec rcx
jnz .forDigits
;---------------------
cmp WQ, 0x7fffffff    ; Check for 31-bit overflow. At this point,
ja mErr6Overflow      ;   valid accumulator range is 0..(2^32)-1
jmp .done             ; making it here means successful decimal conversion
;/////////////////////
.hex:                 ; Attempt to convert as 32-bit hex number
xor WQ, WQ            ; zero accumulator
;---------------------
.forHexDigits:        ; Attempt to convert word as unsigned hex number
mov r9b, [rdi+r8]     ; get next byte of word (maybe digit, or maybe not)
sub r9b, '0'          ; attempt to convert from ASCII digit to uint8
cmp r9b, 9
jbe .goodHexDigit
;---------------------
                      ; Attempt to convert from A..F to 10..15
sub r9b, 7            ; 'A'-'0' = 17 --> 'A'-'0'-7 = 10
cmp r9b, 10           ; set r10b if digit >= 'A'
setae r10b
cmp r9b, 15           ; set r11b if digit <= 'F'
setbe r11b
test r10b, r11b       ; jump if ((digit >= 'A') && (digit <= 'F'))
jnz .goodHexDigit
;---------------------
                      ; Attempt to convert from a..f to 10..15
sub r9b, 32           ; 'a'-'0' = 49 --> 'a'-'0'-7-32 = 10
cmp r9b, 10           ; set r10b if digit >= 'a'
setae r10b
cmp r9b, 15           ; set r11b if digit <= 'f'
setbe r11b
test r10b, r11b       ; jump if ((digit < 'a') || (digit > 'f'))
jz .doneNaN
;---------------------
.goodHexDigit:
imul WQ, 16           ; scale accumulator
add WQ, r9            ; add value of digit to accumulator
jo mErr6Overflow      ; check for 64-bit overflow now (32-bit comes later)
inc r8                ; keep looping until end of word
dec rcx
jnz .forHexDigits
;---------------------
xor rcx, rcx          ; prepare rcx with 0x00000000fffffffff
dec ecx
cmp WQ, rcx           ; check accumulator for 32-bit overflow
ja mErr6Overflow
                      ; making it here means successful conversion, so..
xor r10, r10          ; clear sign flag (r10b) and continue to .done
;/////////////////////
.done:                   ; Conversion okay, so adjust sign and push to stack
mov r11, WQ              ; prepare twos-complement negation of accumulator
neg r11
test r10b, r10b          ; check if the negative flag was set for a '-'
cmovnz WQ, r11           ; ...if so, swap accumulator value for its negative
jmp mPush                ; push the number
;------------------------
.doneNaN:                ; failed conversion, signal NaN
or VMFlags, VMNaN
ret
;------------------------
.doneErr1:
jmp mErr5NumberFormat
;------------------------
.doneErr2:
jmp mErr5NumberFormat


mCompileLiteral:         ; Compile T to heap with integer literal token prefix
cmp T, 0                 ; dispatch to most appropriate size of literal
jl .negative
cmp T, 255
jbe .u8                  ; number fits in 0..255     --> use U8
cmp T, 65536
jbe .u16                 ; fits in 256..65535        --> use U16
jmp .i32                 ; fits in 65535..2147483647 --> use I32
.negative:
cmp T, -127
jge .i8                  ; fits in -127..-1     --> use I8
cmp T, -32768
jge .i16                 ; fits in -32768..-128 --> use I16 (otherwise, I32)
;...                     ; fall through to .i32
;-----------------------
.i32:                    ; Compile T as 32-bit signed literal with i32 token
fPush tI32,       .end   ; -> {S: number, T: tI32 (token)}
fDo   CompileU8,  .end   ; -> {T: number}
fDo   CompileU32, .end   ; -> {}
ret
;-----------------------
.u8:                     ; Compile T as 8-bit unsigned literal with u8 token
fPush tU8,        .end   ; -> {S: number, T: tU8 (token)}
fDo   CompileU8,  .end   ; -> {T: number}
fDo   CompileU8,  .end   ; -> {}
ret
;-----------------------
.u16:                    ; Compile T as 16-bit unsigned literal with u16 token
fPush tU16,       .end   ; -> {S: number, T: tU16 (token)}
fDo   CompileU8,  .end   ; -> {T: number}
fDo   CompileU16, .end   ; -> {}
ret
;-----------------------
.i8:                     ; Compile T as 8-bit signed literal with i8 token
fPush tI8,        .end   ; -> {S: number, T: tI8 (token)}
fDo   CompileU8,  .end   ; -> {T: number}
fDo   CompileU8,  .end   ; -> {}
ret
;-----------------------
.i16:                    ; Compile T as 16-bit signed literal with i16 token
fPush tI16,       .end   ; -> {S: number, T: tI16 (token)}
fDo   CompileU8,  .end   ; -> {T: number}
fDo   CompileU16, .end   ; -> {}
ret
;-----------------------
.end:
ret


mCompileU8:              ; Compile U8 (1 byte) from T to end of heap
fDo   Here,       .end   ; -> {S: u8 value, T: [DP]}
fDo   ByteStore,  .end   ; -> {}                             (store 8-bit byte)
fDo   Here,       .end   ; -> {T: [DP] (before ByteStore)}
fDo   OnePlus,    .end   ; -> {T: [DP]+1}
fPush DP,         .end   ; -> {T: DP}
fDo   WordStore,  .end   ; -> {}                                (add 1 to [DP])
.end:
ret

mCompileU16:             ; Compile U16 (2 bytes) from T to end of heap
fDo   Here,       .end   ; -> {S: u16 value, T: [DP]}
fDo   WordStore,  .end   ; -> {}                            (store 16-bit word)
fDo   Here,       .end   ; -> {T: [DP] (before WordStore)}
fDo   TwoPlus,    .end   ; -> {T: [DP]+2}
fPush DP,         .end   ; -> {T: DP}
fDo   WordStore,  .end   ; -> {}                                (add 2 to [DP])
.end:
ret

mCompileU32:             ; Compile U32 (4 bytes) from T to end of heap
fDo   Here,       .end   ; -> {S: u32 value, T: [DP]}
fDo   Store,      .end   ; -> {}                           (store 32-bit dword)
fDo   Here,       .end   ; -> {T: [DP] (before WordStore)}
fDo   FourPlus,   .end   ; -> {T: [DP]+4}
fPush DP,         .end   ; -> {T: DP}
fDo   WordStore,  .end   ; -> {}                                (add 4 to [DP])
.end:
ret

mHere:                   ; HERE -- Push address of first free dictionary byte
fPush DP,        .end    ; -> {T: DP (address of DP)}
fDo   WordFetch, .end    ; -> {T: [DP] (address of first free dictionary byte)}
.end:
ret

mLast:                   ; LAST -- Push address of last dictionary item pointer
fPush Last,      .end    ; -> {T: Last (address of pointer to last item)}
.end:
ret


mIf:                     ; IF -- Jump forward to EndIf if T is non-zero
test VMFlags, VMCompile  ; in compile mode, jump to the compiler
jnz mCompileIf
; ----------
; TODO: figure out how to distinguish between this being invoked by the
;       outer interpreter with compile mode on, the outer interpreter with
;       compile mode off, or by the inner interpreter when running a compiled
;       word. This shouldn't be available from the outer interpreter when not
;       compiling, but I don't have a good mechanism to prevent that yet.
; -----------
fDo  PopW,        .end   ; -> {}, {W: n (the value to be tested)}
test W, W                ; if value is 0, do not jump to ELSE/EndIf
jz .doTrue               ; CAUTION! this jz is inverted from the VM instruction
.doElse:
movzx edi, word [Mem+ebp] ; load jump target virtual address
mov ebp, edi             ; jump to ELSE/EndIf
ret
.doTrue:
add ebp, 2               ; advance instruction pointer past jump address
.end:
ret


mElse:                   ; ELSE -- doesn't do anything except when compiling
test VMFlags, VMCompile  ; in compile mode, jump to the compiler
jnz mCompileElse
jmp mErr30CompileOnlyWord


mEndIf:                  ; EndIf -- doesn't do anything except when compiling
test VMFlags, VMCompile  ; in compile mode, jump to the compiler
jnz mCompileEndIf
jmp mErr30CompileOnlyWord


; The IF ... ELSE ... EndIf works by compiling temporary jump target addresses,
; then pushing a pointer to the jump target address onto the data stack so it
; can be patched later when the actual jump target is known. IF pushes a
; pointer to its temporary jump target so the address can be patched by ELSE or
; EndIf. ELSE patches IF's jump address, compiles a jump to EndIf (temporary
; address), then pushes a pointer for EndIf to patch the address. EndIf just
; patches the address for IF or ELSE. EndIf doesn't care which one it was
; because the exact same action works for both options.

mCompileIf:              ; Compile if token+addr, push pointer for ELSE/EndIf
fPush  tIf,       .end   ; -> {T: tIf}
fDo    CompileU8, .end   ; -> {}
fDo    Here,      .end   ; -> {T: [DP] (jump addr gets patched by else/endif)}
fPush  0,         .end   ; -> {S: [CodeP], 0 (temp jump addr)}
fDo    CompileU16, .end  ; -> {S: [CodeP]}       (compile temporary jump addr)
.end:
ret


mCompileElse:            ; ELSE -- patch IF's addr, push addr for EndIf, ...
fPush  tJump,     .end   ; -> {S: [DP] (if), T: tJump}  (compile jump tok)
fDo    CompileU8, .end   ; -> {T: [DP] (if)}
fDo    Here,      .end   ; -> {S: [DP] (else jump address pointer)}
;...                     ; Compile a temporary jump address for EndIf to patch
fPush  0,         .end   ;  -> {[DP] (if), S: [CDP] (else), T: 0}
fDo    CompileU16, .end  ;  -> {S: [DP] (if), T: [DP] (else)}
;...                     ; Patch IF's jump target to current code pointer
fDo    Swap,      .end   ;  -> {S: [DP] (else), T: [DP] (if)}

fDo    Here,      .end   ;  -> {[DP] (else), S: [DP] (if), T: [DP] (now)}
fDo    Swap,      .end   ;  -> {[DP] (else), S: [DP] (now), T: [DP] (if)}
fDo    WordStore, .end   ;  -> {T: [DP] (else, jump addr for EndIf to patch)}
.end:
ret


mCompileEndIf:           ; EndIf -- patch IF or ELSE's jump address
fDo    Here,      .end   ; -> {S: [DP] (old, to be patched), T: [DP] (now)}
fDo    Swap,      .end   ; -> {S: [DP] (now), T: [DP] (old)}
fDo    WordStore, .end   ; -> {}
.end:
ret


mFor:                    ; FOR -- Start a FOR..NEXT loop
test VMFlags, VMCompile  ; stop if not in compile mode
jz mErr30CompileOnlyWord
;-----------------------
fPush tToR,       .end   ; -> {T: tToR}   (>R token to use T as loop counter)
fDo   CompileU8,  .end   ; -> {}
fDo   Here,       .end   ; -> {T: [DP]}   (push jump target address for NEXT)
.end:
ret

mNext:                   ; NEXT -- If R is 0, drop R; else decrement R and jump
test VMFlags, VMCompile  ; if in compile mode, jump to the compiler
jnz .compileNext
;-----------------------
movq rdi, RSDeep         ; make sure return stack is not empty
cmp dil, 1
jb mErr20ReturnUnderflow
test R, R                ; check if R is 0
jz .endOfLoop
;-----------------------
.decAndJump:             ; else: decrement R and jump
dec R
movzx edi, word [Mem+ebp]  ; get the jump address (follows tNext token)
mov ebp, edi             ; set I (ebp) to jump address
ret
;-----------------------
.endOfLoop:
add ebp, 2               ; advance I (ebp) past the jump address
jmp mRPopW               ; drop R
;-----------------------
.compileNext:            ; Compile a tNext token with address of FOR
fPush tNext,      .end   ; {S: [CodeP] (from FOR), T: tNext}
fDo   CompileU8,  .end   ; {T: [CodeP] (from FOR)}
fDo   CompileU16, .end   ; {}   (compile the address for backwards jump to FOR)
.end:
ret
