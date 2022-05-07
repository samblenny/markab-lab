; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; libmarkab implements inner and outer interpreters of the Markab Forth system.
;
; Sample output:
; ```
; Markab v0.0.1
; type 'bye' or ^C to exit
;  __  __          _        _
; |  \/  |__ _ _ _| |____ _| |__
; | |\/| / _` | '_| / / _` | '_ \
; |_|  |_\__,_|_| |_\_\__,_|_.__/

;   1 2 3
;   7  OK
; foo  Err7 Not found [dec]: foo
; : fOo  OK
; foo  OK
; FOO  OK
; bye  OK
; ```

bits 64
default rel
global markab_cold
global markab_outer

extern mkb_host_write
extern mkb_host_step_stdin

;=============================
section .data
;=============================

;-----------------------------
; Codegen Import
;
; This includes:
; 1. `%define t...` defines for VM instruction token values (e.g. tNext)
; 2. Jump table including:
;    - JumpTable: label for start of jump table
;    - JumpTableLen: define for length of jump table (to check valid tokens)
; 3. Data structure for dictionary 0 (built-in words):
;    - Dct0Head: head of dictionary linked list
;
%include "generated_data.nasm"


;-----------------------------
; Loadscreen

align 16, db 0
db "== LoadScreen =="
align 16, db 0
Screen0:                   ; load screen text (text with Forth source code)
incbin "screen00.fs"
EndScreen0: db 0
align 16, db 0
Screen0Len:                ; length of load screen text in bytes
dd (EndScreen0 - Screen0)


;-----------------------------
; Strings {dword len, chars}

align 16, db 0
db "== VM Strings =="

%macro mkStr 1             ; Make string with a 2-byte length prefix
  %strlen %%mStrLen %1     ; calculate length string
  dw %%mStrLen             ; 2 byte length
  db %1                    ; <length> bytes of string
%endmacro

datVersion:  mkStr `Markab v0.0.1\ntype 'bye' or ^C to exit\n`
datErr1se:   mkStr "  E1 Stack underflow"
datErr2sf:   mkStr "  E2 Stack full"
datErr3bt:   mkStr "  E3 Bad token: "
datErr4nq:   mkStr `  E4 Expected \"`
datErr5af:   mkStr "  E5 Assertion failed: "
datErr6of:   mkStr "  E6 Overflow: "
datErr7nfd:  mkStr "  E7 ? "
datErr7nfh:  mkStr "  E7 [hex] ? "
datErr8np:   mkStr "  E8 Expected )"
datErr9df:   mkStr "  E9 Dictionary full"
datErr10en:  mkStr "  E10 Expected name"
datErr11ntl: mkStr "  E11 Name too long"
datErr12dbz: mkStr "  E12 Divide by 0"
datDotSNone: mkStr "  Stack is empty"
datOK:       mkStr `  OK\n`

align 16, db 0
db "=== End.data ==="

;=============================
section .bss
;=============================

%macro byteBuf 2
  align 16, resb 0
  %1: resb %2
  align 16, resb 0
%endmacro
%macro dwordBuf 2
  align 16, resb 0
  %1: resd %2
  align 16, resb 0
%endmacro
%macro dwordVar 1
  align 4, resb 0
  %1: resd 1
%endmacro

%define DSMax 17              ; total size of data stack (T + 16 dwords)
%define RSMax 16              ; total size of return stack (16 dwords)
%define StrMax 1022           ; length of string data area
%define DctMax 16384          ; dictionary length = 16 KiB

dwordBuf DSBase, DSMax-1      ; data stack (size excludes top item T)
dwordBuf RSBase, RSMax        ; Return stack for token interpreter

byteBuf Pad, 2+StrMax         ; string scratch buffer; word 0 is length
byteBuf PadRtl, StrMax+2      ; right-to-left buffer for formatting numbers;
                              ; word [PadRtl+StrMax] is index first used byte

byteBuf Dct2, DctMax          ; Start of user dictionary (Dictionary 2)
dwordVar DP                   ; index to next free byte of dictionary
dwordVar Last                 ; pointer to head of dictionary

dwordVar TibPtr               ; Pointer to terminal input buffer (TIB)
dwordVar TibLen               ; Length of TIB (count of max available bytes)
dwordVar IN                   ; Index into TIB of next available input byte
dwordVar Base                 ; Number base for numeric string conversions



;=============================
section .text
;=============================

;-----------------------------
; Debug macro

%macro DEBUG 1
  push rax
  push rbx
  push rcx
  push rbp
  push rdx
  push rsi
  push rdi
  mov WB, %1
  call mEmit.W
  pop rdi
  pop rsi
  pop rdx
  pop rbp
  pop rcx
  pop rbx
  pop rax
%endmacro


;-----------------------------
; Stack macros

%define W eax                 ; Working register, 32-bit zero-extended dword
%define WQ rax                ; Working register, 64-bit qword (for pointers)
%define WW ax                 ; Working register, 16-bit word
%define WB al                 ; Working register, low byte

%define VMBye 1               ; Bye bit: set means bye has been invoked
%define VMErr 2               ; Err bit: set means error condition
%define VMNaN 4               ; NaN bit: set means number conversion failed
%define VMFlags r12b          ; Virtual machine status flags

%define T r13d                ; Top on stack, 32-bit zero-extended dword
%define TB r13b               ; Top on stack, low byte
%define DSDeep r14d           ; Current depth of data stack (including T)
%define DSDeepB r14d          ; Current depth of data stack, low byte
%define RSDeep r15d           ; Current depth of return stack
%define RSDeepB r15d          ; Current depth of return stack, low byte


;-----------------------------
; Library init entry point

markab_cold:
enter 0, 0
xor W, W                      ; init data stack registers
mov T, W
mov DSDeep, W
mov RSDeep, W
mov [Pad], W
mov [Last], dword Dct0Head    ; dictionary head starts at head of Dct0
mov [DP], W                   ; dictionary pointer starts at 0
call mDecimal                 ; default number base
xor VMFlags, VMFlags          ; clear VM flags
lea W, datVersion             ; print version string
call mStrPut.W
;-----------------------------
mov edi, Screen0              ; Interpret loadscreen
mov esi, [Screen0Len]
call markab_outer             ; markab_outer(edi: *buf, esi: count)
;-----------------------------
.OuterLoop:                   ; Begin outer loop
test VMFlags, VMBye           ; Break loop if bye flag is set
jnz .done
push rbp                      ; align stack to 16 bytes
mov rbp, rsp
and rsp, -16
call mkb_host_step_stdin      ; step the non-blocking stdin state machine
mov rsp, rbp                  ; restore stack to previous alignment
pop rbp
test rax, rax
jz .OuterLoop                 ; loop until return value is non-zero
.done:
leave
ret

;-----------------------------
; Interpreters

doInner:                      ; Inner interpreter (rdi: tokenLen, rsi:tokenPtr)
push rbp
push rbx
mov rbp, rsi                  ; ebp = instruction pointer (I)
mov rbx, rdi                  ; ebx = max loop iterations
;/////////////////////////////
.for:
movzx W, byte [rbp]           ; load token at I
cmp WB, tNext                 ; handle `Next` specially
je .done
cmp WB, JumpTableLen          ; detect token beyond jump table range (CAUTION!)
jae .doneBadToken
lea edi, [JumpTable]          ; fetch jump table address
mov esi, dword [rdi+4*WQ]
inc ebp                       ; advance I
call rsi                      ; jump (callee may adjust I for LITx)
dec ebx
jnz .for                      ; loop until end of token count (or Next)
;/////////////////////////////
.done:                        ; normal exit path
pop rbx
pop rbp
ret
;-----------------------------
.doneBadToken:                ; exit path for invalid token
call mErr3BadToken
pop rbx
pop rbp
ret


; Interpret a line of text from the input stream
;  void markab_outer(edi: u8 *buf, esi: u32 count)
markab_outer:
test esi, esi            ; end now if input buffer is empty
jz .done
;////////////////////////
                         ; Store arguments (approximate figForth TIB and IN)
mov [TibPtr], edi        ; edi: pointer to input stream byte buffer
mov [TibLen], esi        ; esi: count of available bytes in the input stream
mov [IN], dword 0        ; index into TIB of next available input byte
;////////////////////////
.forNextWord:            ; Look ahead to find bounds of next word
test VMFlags, VMBye      ; stop looking if bye flag is set
jnz .done
mov edi, [TibPtr]        ; edi = TIB base pointer
mov ecx, [TibLen]        ; ecx = TIB length
mov WB, 1                ; assert(TibLen <= StrMax, 1)
cmp ecx, StrMax
jnle mErr5Assert
mov esi, [IN]            ; esi = IN (index to next available byte of TIB)
cmp esi, ecx             ; stop looking if IN >= TibLen (ran out of bytes)
jnl .done
sub ecx, esi             ; ecx = TibLen - IN  (count of available bytes)
;------------------------
.forScanStart:           ; Skip spaces to find next word-start boundary
mov WB, byte [rdi+rsi]   ; check if current byte is non-space
cmp WB, ' '              ; calculate r10b = ((WB==' ')||(WB==10)||(WB==13))
sete r10b
cmp WB, 10               ; check for LF
sete r11b
or r10b, r11b
cmp WB, 13               ; check for CR
sete r11b
or r10b, r11b            ; r10b will be set if WB is in (' ', LF, CR)
jz .forScanEnd           ; jump if r10b is set (found word-start boundary)
inc esi                  ; advance esi past the ' '
mov [IN], esi            ; update IN (save index to start of word)
dec ecx                  ; loop if there are more bytes
jnz .forScanStart
jmp .done                ; jump if reached end of TIB (it was all spaces)
;------------------------
.forScanEnd:             ; Scan for space or end of stream (word-end boundary)
mov WB, byte [rdi+rsi]
cmp WB, ' '              ; check for space
sete r10b
cmp WB, 10               ; check for LF
sete r11b
or r10b, r11b
cmp WB, 13               ; check for CR
sete r11b
or r10b, r11b            ; r10b will be set if WB is in (' ', LF, CR)
jnz .wordSpace
dec ecx                  ; loop if there are more bytes (detect end of stream)
jz .wordEndBuf
inc esi                  ; advance index
jmp .forScanEnd
;////////////////////////
                         ; Handle word terminated by space
.wordSpace:              ; currently, IN is start index, esi is end index + 1
mov W, [IN]              ; prepare arguments for calling doWord:
sub esi, W               ; convert esi from index_of_word_end to word_length
add edi, W               ; convert edi from TibPtr to start_of_word_pointer
add W, esi               ; update IN to point 1 past the space
inc W
mov [IN], W
call doWord              ; void doWord(rdi: u8 *buf, rsi: count)
test VMFlags, VMErr      ; non-zero VMErr flag means there was error (hide OK)
jnz .doneErr
jmp .forNextWord
;------------------------
                         ; Handle word terminated by CR (end of stream)
.wordEndBuf:             ; word is [rdi]..[rdi+rcx] (there was no space)
mov W, [IN]              ; prepare arguments for calling doWord:
sub esi, W               ; convert esi from index_of_word_end to word_length
inc esi
add edi, W               ; convert edi from TibPtr to start_of_word_pointer
call doWord              ; void doWord(rdi: u8 *buf, rsi: count)
test VMFlags, VMErr      ; non-zero VMErr flag means there was error (hide OK)
jz .done                 ; ...in that case, fall through to .doneErr
;////////////////////////
.doneErr:                ; Print CR (instead of OK) and clear the error bit
and VMFlags, (~VMErr)
jmp mCR
;------------------------
.done:                   ; Print OK for success
lea W, [datOK]
jmp mStrPut.W

; Attempt to do the action for a word, potentially including:
;   1. Interpret it according to the dictionary
;   2. Push it to the stack as a number
;   3. Print an error message
; doWord(rdi: u8 *buf, rsi: countWord, rdx: countMax)
; rdi: pointer to input stream buffer
; rsi: count of bytes in this word
; rdx: count of maximum available bytes in input stream (for compiling words)
doWord:
push rbp              ; save arguments
push rbx
mov rbp, rdi          ; rbp = *buf
mov rbx, rsi          ; rbx = count
mov edi, [Last]       ; Load head of dictionary list. Struct format is:
                      ; {dd .link, db .nameLen, .name, db .tokenLen, .tokens}
;/////////////////////
.lengthCheck:
xor W, W              ; Check: does .nameLen match the search word length?
mov WB, byte [rdi+4]  ; WB = .nameLen (length of current dictionary item .name)
cmp WB, bl            ; compare WB to search word length (count)
jnz .nextItem         ; ...not a match, so follow link and check next entry
.lengthMatch:
movzx rcx, bl         ; rcx = length of dict word (same length as search word)
xor W, W              ; for(i=0; search[i]==dictName[i] && i<count; i++)
;/////////////////////
.for:
mov dl, [rbp+WQ]      ; load dl = search[i]
cmp dl, 'A'           ; convert dl (search[i]) to lowercase without branching
setae r10b
cmp dl, 'Z'
setbe r11b
test r10b, r11b
setnz r10b            ; ...at this point, r10 is set if dl is in 'A'..'Z'
mov r11b, dl          ; ...speculatively prepare a lowercase (c+32) character
add r11b, 32
test r10b, r10b       ; ...swap in the lowercase char if dl was uppercase
cmovnz dx, r11w
lea rsi, [rdi+5]      ; check dl == dictName[i]
cmp dl, [rsi+WQ]
jnz .nextItem         ; break if bytes don't match
inc W                 ; otherwise, continue
dec rcx
jnz .for              ; no more bytes to check means search word matches name
;/////////////////////
.wordMatch:           ; got a match, rsi+W now points to .tokenLen
lea rcx, [rdi+5]      ; rcx = pointer to .tokenLen (skip .link and .nameLen)
add rcx, WQ           ; ...(skip .name)
xor rdi, rdi          ; rdi = value of .tokenLen
mov dil, byte [rcx]
inc rcx               ; rsi = pointer to .tokens
mov rsi, rcx
call doInner          ; doInner(rdi: tokenLen, rsi: tokensPointer)
jmp .done
;/////////////////////
.nextItem:            ; follow link;
mov W, [rdi]          ; check for null pointer (tail of list)
test W, W
jz .wordNotFound
mov edi, W            ; rdi = pointer to next item in dictionary list
jmp .lengthCheck      ; continue with match-checking loop
;/////////////////////
.wordNotFound:
mov rdi, rbp          ; prepare args for mNumber(edi: *buf, esi: count)
mov rsi, rbx
call mNumber          ; attempt to convert word as number
test VMFlags, VMNaN   ; check if it worked
jz .done
and VMFlags, (~VMNaN) ; ...if not, clear the NaN flag and show an error
lea W, [datErr7nfd]   ; load err not found error message (decimal version)
lea edx, [datErr7nfh] ; swap error message for hex version if base is 16
mov ecx, [Base]
cmp cl, 16
cmove W, edx
call mStrPut.W        ; print the not found error prefix
mov rdi, rbp          ; print the word that wasn't found
mov rsi, rbx
call mStrPut.RdiRsi
or VMFlags, VMErr     ; return with error condition
;/////////////////////
.done:
pop rbx               ; restore registers
pop rbp
ret
;/////////////////////


;-----------------------------
; Dictionary: Error handling

mErrPutW:                     ; Print error from W and set error flag
call mStrPut.W
; continue to mErr
mErr:
or VMFlags, VMErr             ; set error condition flag (hide OK prompt)
ret

mErr1Underflow:               ; Error 1: Stack underflow
lea W, [datErr1se]
jmp mErrPutW

mErr2Overflow:                ; Error 2: Stack overflow
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

mErr5Assert:                  ; Error 5: assertion failed (W: error code)
push WQ
lea W, [datErr5af]            ; print error message
call mStrPut.W
pop WQ
call mDot.W                   ; print error code
or VMFlags, VMErr             ; set error condition flag (hide OK prompt)
ret                           ; return control to interpreter

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


;-----------------------------
; Dictionary: Literals
;
; These are designed for efficient compiling of tokens to push signed 32-bit
; numeric literals onto the stack for use with signed 32-bit math operations.
;

mU8:                          ; Push zero-extended unsigned 8-bit literal
movzx W, byte [rbp]           ; read literal from token stream
inc ebp                       ; ajust I
jmp mPush

mI8:                          ; Push sign-extended signed 8-bit literal
movsx W, byte [rbp]           ; read literal from token stream
inc ebp                       ; ajust I
jmp mPush

mU16:                         ; Push zero-extended unsigned 16-bit literal
movzx W, word [rbp]           ; read literal from token stream
add ebp, 2                    ; adjust I
jmp mPush

mI16:                         ; Push sign-extended signed 16-bit literal
movsx W, word [rbp]           ; read literal from token stream
add ebp, 2                    ; adjust I
jmp mPush

mI32:                         ; Push signed 32-bit dword literal
mov W, dword [rbp]            ; read literal from token stream
add ebp, 4                    ; adjust I
jmp mPush

mDotQuoteC:                   ; Print string literal to stdout (token compiled)
movzx ecx, word [rbp]         ; get length of string in bytes (for adjusting I)
add cx, 2                     ;   add 2 for length dword
mov W, ebp                    ; I (ebp) should be pointing to {length, chars}
add ebp, ecx                  ; adjust I past string
jmp mStrPut.W

; Print a string literal from the input stream to stdout (interpret mode)
; input bytes come from TibPtr using TibLen and IN
mDotQuoteI:
mov edi, [TibPtr]        ; edi = TIB base pointer
mov esi, [TibLen]        ; esi = TIB length
mov ecx, [IN]            ; ecx = IN (index to next available byte of TIB)
cmp esi, ecx             ; stop looking if TibLen <= IN (ran out of bytes)
jna mErr4NoQuote
;------------------------
.forScanQuote:           ; Find a double quote (") character
mov WB, [rdi+rcx]        ; check if current byte is '"'
cmp WB, '"'
jz .done                 ; if so, stop looking
inc ecx                  ; otherwise, advance the index
cmp esi, ecx             ; loop if there are more bytes
jnz .forScanQuote
jmp mErr4NoQuote         ; reaching end of TIB without a quote is an error
;------------------------
.done:
mov W, [IN]
mov esi, ecx             ; ecx-[IN] is count of bytes copied to Pad
sub esi, W
add edi, W               ; edi = [TibPtr] + (ecx-[IN]) (old edi was [TibPtr])
inc ecx                  ; store new IN (skip string and closing '"')
mov [IN], ecx
jmp mStrPut.RdiRsi       ; print it: mStrPut.RdiRsi(rdi: *buf, rsi: count)

; Paren comment: skip input text until ")"
;   input bytes come from TibPtr using TibLen and IN
mParen:
mov edi, [TibPtr]        ; edi = TIB base pointer
mov esi, [TibLen]        ; esi = TIB length
mov ecx, [IN]            ; ecx = IN (index to next available byte of TIB
cmp esi, ecx             ; stop looking if TibLen <= IN (ran out of bytes)
jna mErr8NoParen
;------------------------
.forScanParen:
mov WB, [rdi+rcx]        ; check if current byte is ')'
cmp WB, ')'
jz .done                 ; if so, stop looking
inc ecx                  ; otherwise, continue with next byte
cmp esi, ecx             ; loop until index >= TibLen
ja .forScanParen
jmp mErr8NoParen         ; oops, end of TIB reached without finding ")"
;------------------------
.done:
inc ecx
mov [IN], ecx
ret

;-----------------------------
; Compiling words

mColon:                       ; COLON - define a word
mov edi, [DP]                 ; load dictionary pointer (index into Dct2)
push rdi                      ; save [DP] in case we want to roll back changes
call mCreate                  ; add name from input stream to dictionary
test VMFlags, VMErr           ; stop and roll back dictionary if it failed
jnz .doneErr
;-----------------------------
mov esi, [DP]                 ; load dictionary pointer (updated by create)
mov W, esi                    ; check if there is room to add tokens
add W, 2
cmp W, DctMax                 ; if not, stop with an error
jnb .doneErrFull
mov [Dct2+esi], byte 1        ; otherwise, append {.tokenLen: 1}
inc esi
mov [Dct2+esi], byte tNext    ; append {.tokens: tNext}
inc esi
mov [DP], esi
;-----------------------------
.done:
pop rdi                       ; commit dictionary changes
lea W, [Dct2+edi]
mov [Last], W
ret
;-----------------------------
.doneErrFull:
pop rdi                       ; roll back dictionary changes
mov [DP], edi
jmp mErr9DictFull             ; show error message
;-----------------------------
.doneErr:
pop rdi                       ; roll back dictionary changes
mov [DP], edi
ret

; CREATE - Add a name to the dictionary
; struct format is: {dd .link, db .nameLen, <name>, db .tokenLen, .tokens}
mCreate:
mov edi, Dct2            ; load dictionary base address
mov esi, [DP]            ; load dictionary pointer (index relative to Dct2)
mov W, esi               ; check if dictionary has room for a link (4 bytes)
add W, 4
cmp W, DctMax            ; stop if dictionary is full
jnb mErr9DictFull
;------------------------
push rsi                 ; save a copy of [DP] to use for rollback if needed
mov W, [Last]            ; append {.link: [Last]} to dictionary
mov [edi+esi], W
add esi, 4               ; update [DP]
mov [DP], esi
push rsi                 ; store pointer to {.nameLen, <name>}
call mWord               ; append word from [TIB+IN] as {.nameLen, <name>}
pop rsi                  ; load pointer to {.nameLen, <name>}
test VMFlags, VMErr      ; check for errors
jnz .doneErr
;------------------------
                         ; Lowercase the name so case-insensitive lookups work
lea rdi, [Dct2+rsi]      ; prepare {rdi, rsi} args for the name that was just
movzx rsi, byte [rdi]    ;   stored at [Dct2+DP] (for DP from before mWord)
inc rdi                  ; skip the length byte of {db .nameLen, <name>}
call mLowercase          ; mLowercase(rdi: *buf, rsi: count)
;------------------------
.done:
pop rsi                  ; commit dictionary changes
ret
;------------------------
.doneErr:
pop rsi                  ; roll back dictionary changes
mov [DP], esi
ret

; WORD - Copy a word from [TIB+IN] to [Dct2+DP]
mWord:
mov edi, [TibPtr]        ; load input bufffer base pointer
mov esi, [IN]            ; load input buffer index
mov ecx, [TibLen]        ; load input buffer length
cmp rsi, rcx             ; stop if no bytes are available in input buffer
jnb .doneErr
;////////////////////////
.forScanStart:           ; Skip spaces to find next word-start boundary
mov WB, byte [rdi+rsi]   ; check if current byte is non-space
cmp WB, ' '              ; calculate r10b = ((WB==' ')||(WB==10)||(WB==13))
sete r10b
cmp WB, 10               ; check for LF
sete r11b
or r10b, r11b
cmp WB, 13               ; check for CR
sete r11b
or r10b, r11b            ; r10b will be set if WB is in (' ', LF, CR)
jz .forScanEnd           ; jump if word-start boundary was found
inc rsi                  ; otherwise, advance past the ' '
mov [IN], esi            ; update IN (save index to start of word)
cmp rsi, rcx             ; loop if there are more bytes
jb .forScanStart
jmp .doneErr             ; jump if reached end of TIB (it was all spaces)
;------------------------
.forScanEnd:             ; Scan for space or end of stream (word-end boundary)
mov WB, byte [rdi+rsi]
cmp WB, ' '              ; check for space
sete r10b
cmp WB, 10               ; check for LF
sete r11b
or r10b, r11b
cmp WB, 13               ; check for CR
sete r11b
or r10b, r11b            ; r10b will be set if WB is in (' ', LF, CR)
jnz .wordSpace
inc rsi
cmp rsi, rcx
jb .forScanEnd           ; loop if there are more bytes (detect end of stream)
;////////////////////////
                         ; Handle word terminated by end of stream
.wordEndBuf:             ; currently: {[IN]: start index, rsi: end index}
mov W, [IN]              ; prepare arguments for calling doWord:
sub esi, W               ; convert esi from index_of_word_end to word_length
add edi, W               ; convert edi from TibPtr to start_of_word_pointer
mov W, [IN]              ; update [IN]
add W, esi
mov [IN], W
jmp .copyWordRdiRsi
;------------------------
                         ; Handle word terminated by space, LF, or CR
.wordSpace:              ; currently: {[IN]: start index, rsi: end index + 1}
mov W, [IN]              ; prepare word as {rdi: *buf, rsi: count}
sub esi, W               ; convert rsi from index_of_word_end to word_length
add edi, W               ; convert rdi from TibPtr to start_of_word_pointer
add W, esi               ; update IN to point 1 past the space
inc W
mov [IN], W              ; (then fall through to copy word)
;////////////////////////
.copyWordRdiRsi:         ; Copy word {rdi: *buf, rsi: count} to [Dct2+[DP]]
mov r8d, Dct2            ; load dictionary base address
mov r9d, [DP]            ; load dictionary pointer (index relative to Dct2)
mov W, r9d               ; check if dictionary has room for the name
inc W                    ;  add 1 for {.nameLen: <byte>}
add W, esi               ;  add byte count for {.name: <name>}
cmp W, DctMax            ; stop if dictionary is full
jnb mErr9DictFull
mov W, esi               ; stop if word is too long (max 255 bytes)
cmp W, 255
ja mErr11NameTooLong
;------------------------
mov [r8d+r9d], sil       ; store {.nameLen: <byte count>}
inc r9d
xor rcx, rcx             ; zero source index
.forCopy:
mov WB, [rdi+rcx]        ; load [TIB+IN+rcx]
mov [r8d+r9d], WB        ; store [Dct2+DP+rcx] (one byte of {.name: <name>})
inc rcx                  ; keep looping while i<rsi
inc r9d
cmp rcx, rsi
jb .forCopy
;////////////////////////
.done:
mov [DP], r9d            ; store the new dictionary pointer
ret
;------------------------
.doneErr:
call mErr10ExpectedName
ret

; Convert string at {rdi: *buf, rsi: count} to lowercase (modify in place)
mLowercase:
xor ecx, ecx
.for:
mov WB, [rdi+rcx]     ; load [rdi+i] for i in 0..(rsi-1)
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
mov [rdi+rcx], WB     ; store the lowercased byte
inc ecx
dec esi               ; loop until all bytes have been checked
jnz .for
ret


;-----------------------------
; Dictionary: Stack ops

mNop:                         ; NOP - do nothing
ret

mNext:                        ; NEXT - (nop) this gets handled by doInner
ret

mDup:                         ; DUP - Push T
cmp DSDeepB, 1
jb mErr2Overflow
mov W, T
jmp mPush

mSwap:                        ; SWAP - Swap T and second item on stack
mov W, DSDeep
cmp WB, 2
jb mErr1Underflow
sub WB, 2
xchg T, [DSBase+4*W]
ret

mOver:                        ; OVER - Push second item on stack
mov W, DSDeep
cmp WB, 2
jb mErr1Underflow
sub WB, 2
mov W, [DSBase+4*W]
jmp mPush

mClearStack:                  ; CLEARSTACK - Drop all stack cells
xor DSDeep, DSDeep
ret

mPush:                        ; PUSH - Push W to data stack
cmp DSDeep, DSMax
jnb mErr2Overflow
mov edi, W                    ; save W before relative address calculation
mov esi, DSDeep               ; calculate store index of old_depth-2+1
dec esi
mov [DSBase+4*esi], T         ; store old value of T
mov T, edi                    ; set T to caller's value of W
inc DSDeep                    ; this depth includes T + (DSMax-1) memory items
ret

mDrop:                        ; DROP - discard (pop) T
cmp DSDeep, 1
jb mErr1Underflow
dec DSDeep                    ; new_depth = old_depth-1
mov W, DSDeep                 ; new second item index = old_depth-2+1-1
dec W
mov T, [DSBase+4*W]
ret


;-----------------------------
; Dictionary: Math ops

mMathDrop:                    ; Shared drop preamble for 2-operand math ops
cmp DSDeep, 2                 ; make sure there are 2 items on the stack
jb mErr1Underflow
mov edi, T                    ; save value of old top item
dec DSDeep                    ; do a drop
mov W, DSDeep
dec W
mov T, [DSBase+4*W]
mov W, edi                    ; leave old T in eax (W) for use with math ops
ret

mPlus:                        ; +   ( 2nd T -- 2nd+T )
call mMathDrop
add T, W
ret

mMinus:                       ; -   ( 2nd T -- 2nd-T )
call mMathDrop
sub T, W
ret

mMul:                         ; *   ( 2nd T -- 2nd*T )
call mMathDrop
imul T, W                     ; imul is signed multiply (mul is unsigned)
ret

mDiv:                         ; /   ( 2nd T -- <quotient 2nd/T> )
call mMathDrop                ; after drop, old value of T is in W
test W, W                     ; make sure divisor is not 0
jz mErr12DivideByZero
cdq                           ; sign extend eax (W, old T) into rax
mov rdi, WQ                   ; save old T in rdi (use qword to prep for idiv)
mov W, T                      ; prepare dividend (old 2nd) in rax
cdq                           ; sign extend old 2nd into rax
idiv rdi                      ; signed divide 2nd/T (rax:quot, rdx:rem)
mov T, W                      ; new T is quotient from eax
ret

mMod:                         ; MOD   ( 2nd T -- <remainder 2nd/T> )
call mMathDrop                ; after drop, old value of T is in W
test W, W                     ; make sure divisor is not 0
jz mErr12DivideByZero
cdq                           ; sign extend eax (W, old T) into rax
mov rdi, WQ                   ; save old T in rdi (use qword to prep for idiv)
mov W, T                      ; prepare dividend (old 2nd) in rax
cdq                           ; sign extend old 2nd into rax
idiv rdi                      ; signed divide 2nd/T (rax:quot, rdx:rem)
mov T, edx                    ; new T is remainder from edx
ret

mDivMod:                      ; /MOD   for 2nd/T: ( 2nd T -- rem quot )
cmp DSDeep, 2                 ; make sure there are 2 items on the stack
jb mErr1Underflow
test T, T                     ; make sure divisor is not 0
jz mErr12DivideByZero
mov ecx, DSDeep               ; fetch old 2nd as dividend to eax (W)
sub cl, 2
mov W, [DSBase+4*ecx]
cdq                           ; sign extend old 2nd in eax to rax
idiv T                        ; signed divide 2nd/T (rax:quot, rdx:rem)
mov edi, eax                  ; save quotient before address calculation
mov esi, edx                  ; save remainder before address calculation
mov [DSBase+4*ecx], esi       ; remainder goes in second item
mov T, edi                    ; quotient goes in top
ret

mMax:                         ; MAX   ( 2nd T -- the_bigger_one )
call mMathDrop
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovl T, W                    ; if old 2nd was less, then use old T for new T
ret

mMin:                         ; MIN   ( 2nd T -- the_smaller_one )
call mMathDrop
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovg T, W                    ; if old 2nd was more, then use old T for new T
ret

mAbs:                         ; ABS   ( T -- abs_of_T )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
mov W, T
neg W                         ; check if negated value of old T is positive
test W, W
cmovns T, W                   ; if so, set new T to negated old T
ret

;-----------------------------
; Dictionary: Boolean ops
;
; == Important Note! ==
; Forth's boolean constants are not like C booleans. In Forth,
;   True value:   0 (all bits clear)
;   False value: -1 (all bits set)
; This allows for sneaky tricks such as using the `AND`, `OR`, `XOR`, and `NOT`
; words to act as both bitwise and boolean operators. Also, false shows up in a
; hexdump as `FFFFFFFF`, so "F's for False" is a good mnemonic.
; =====================

mAnd:                         ; AND   ( 2nd T -- bitwise_and_2nd_T )
call mMathDrop
and T, W
ret

mOr:                          ; OR   ( 2nd T -- bitwise_or_2nd_T )
call mMathDrop
or T, W
ret

mXor:                         ; XOR   ( 2nd T -- bitwise_xor_2nd_T )
call mMathDrop
xor T, W
ret

mNot:                         ; NOT   ( T -- bitwise_negate_T )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
not T
ret

mLess:                        ; <   ( 2nd T -- bool_is_2nd_less_than_T )
call mMathDrop                ; drop leaves old T in W
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to false (-1), assuming 2nd >= T
dec T
xor esi, esi                  ; prepare true (0) in esi
cmp edi, W                    ; test for 2nd < T
cmovl T, esi                  ; if so, change new T to true
ret

mGreater:                     ; >   ( 2nd T -- bool_is_2nd_greater_than_T )
call mMathDrop                ; drop leaves old T in W
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to true (0), assuming 2nd > T
xor esi, esi                  ; prepare false (-1) in esi
dec esi
cmp edi, W                    ; test for 2nd <= T
cmovle T, esi                 ; if so, change new T to false
ret

mEqual:                       ; =   ( 2nd T -- bool_is_2nd_equal_to_T )
call mMathDrop                ; drop leaves old T in W
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to true (0), assuming 2nd = T
xor esi, esi                  ; prepare false (-1) in esi
dec esi
cmp edi, W                    ; test for 2nd <> T   (`<>` means not-equal)
cmovnz T, esi                 ; if so, change new T to false
ret

mZeroLess:                    ; 0<   ( T -- bool_is_T_less_than_0 )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
mov W, T
xor T, T                      ; set T to false (-1)
dec T
xor edi, edi                  ; prepare value of true (0) in edi
test W, W                     ; check of old T<0 by setting sign flag (SF)
cmovs T, edi                  ; if so, change new T to true
ret

mZeroEqual:                   ; 0=   ( T -- bool_is_T_equal_0 )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
mov W, T                      ; save value of T
xor T, T                      ; set T to false (-1)
dec T
xor edi, edi                  ; prepare value of true (0) in edi
test W, W                     ; check if old T was zero (set ZF for W and W)
cmove T, edi                  ; if so, change new T to true
ret


;-----------------------------
; Dictionary: Numbers

mHex:                         ; Set number base to 16
mov [Base], dword 16
ret

mDecimal:                     ; Set number base to 10
mov [Base], dword 10
ret

mNumber:              ; Parse & push i32 from word (rdi: *buf, rsi: count)
mov WB, 3             ; assert count > 0
test rsi, rsi
jz mErr5Assert
;---------------------
xor r8, r8            ; zero index
xor r9, r9            ; zero ASCII digit
mov ecx, esi          ; rcx = count of bytes in word buffer
;---------------------
mov W, [Base]         ; assert that base is 10 or 16 and jump accordingly
cmp WB, 10
jz .decimal           ; use decimal conversion
cmp WB, 16
jz .hex               ; use hex conversion
mov WB, 4
jmp mErr5Assert       ; oops... Base is not valid
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
.done:                ; Conversion okay, so adjust sign and push to stack
mov r11, WQ           ; prepare twos-complement negation of accumulator
neg r11
test r10b, r10b       ; check if the negative flag was set for a '-'
cmovnz WQ, r11        ; ...if so, swap accumulator value for its negative
jmp mPush             ; push the number
;---------------------
.doneNaN:             ; failed conversion, signal NaN
or VMFlags, VMNaN
ret

;-----------------------------
; Dictionary: Strings

mSpace:                ; Print a space to stdout
mov W, ' '
jmp mEmit.W

mCR:                   ; Print a CR (newline) to stdout
mov W, 10
jmp mEmit.W

mEmit:                 ; Print low byte of T as ascii char to stdout
mov W, T
call mDrop
.W:                    ; Print low byte of W as ascii char to stdout
shl W, 24              ; Prepare W as string struct {db 1, 0, ascii, 0}
shr W, 8
mov WB, 1
mov [dword Pad], W     ; Store string struct in Pad
jmp mStrPut


;-----------------------------
; Dictionary: Formatting

mDot:                 ; Print T using number base
cmp DSDeep, 1         ; need at least 1 item on stack
jb mErr1Underflow
mov W, T
push WQ
call mDrop
pop WQ
.W:                   ; Print W using number base
push WQ
call mFmtRtlClear     ; clear number formatting buffer
pop WQ
mov rdi, WQ
call mFmtRtlInt32     ; format W
call mFmtRtlSpace     ; add a space
call mFmtRtlPut       ; print number formatting buffer

mFmtRtlClear:            ; Clear PadRtl with 'x' and reset index to leftmost
mov rcx, StrMax
mov WB, 'x'
.for:
mov [PadRtl+rcx], WB
dec rcx
jnz .for
lea rdi, [PadRtl+StrMax]
mov [rdi], word StrMax
ret

mFmtRtlSpace:            ; Insert (rtl) a space into the PadRtl buffer
xor rdi, rdi
mov dil, ' '
jmp mFmtRtlInsert

mFmtRtlMinus:            ; Insert (rtl) a '-' into the PadRtl buffer
xor rdi, rdi
mov dil, '-'
jmp mFmtRtlInsert

mFmtRtlInsert:           ; Insert (rtl) byte from rdi into the PadRtl buffer
xor rsi, rsi             ; load index of leftmost byte
mov si, [PadRtl+StrMax]
mov WB, 5                ; assert(index <= max valid index value, 5)
cmp si, StrMax
ja mErr5Assert
mov WB, 6                ; assert(index >= min valid index value, 6)
cmp si, 1
jb mErr5Assert
dec rsi                  ; dec rsi to get next available byte
mov [PadRtl+rsi], dil    ; store whatever was in low byte of rdi
mov [PadRtl+StrMax], si  ; store the new leftmost byte index
ret

mFmtRtlPut:              ; Print the contents of PadRtl
xor rcx, rcx             ; rcx = index to leftmost used byte of PadRtl
mov cx, [PadRtl+StrMax]
push rcx
xor rsi, rsi             ; rsi = size of PadRtl (index to rightmost byte + 1)
mov si, StrMax
sub si, cx               ; rsi = count of string bytes in PadRtl
mov WB, 7                ; assert(((rsi >= 0) && (rsi < StrMax)), 7)
cmp si, StrMax
jnb mErr5Assert
pop rcx
lea rdi, [PadRtl+rcx]    ; prepare for mStrPut.RdiRsi(rdi: *buf, rsi: count)
call mStrPut.RdiRsi      ; print format string
jmp mFmtRtlClear         ; clear format string

; Format an int32, right aligned in PadRtl, according to current number base.
;
; This formats a number from rdi into PadRtl, using an algorithm that puts
; digits into the buffer moving from right to left. The idea is to match the
; order of divisions, which necessarily produces digits in least-significant to
; most-significant order. This is intended for building a string from several
; numbers in a row (like with .S).
;
; Arguments: {rdi: number_to_format}
mFmtRtlInt32:
xor rsi, rsi          ; rsi = index to leftmost used byte of PadRtl
mov si, [PadRtl+StrMax]
mov WB, 8             ; assert(index to PadRtl leftmost byte is in range, 8)
cmp rsi, StrMax
ja mErr5Assert
mov WB, 9             ; assert(PadRtl has room for another number, 9)
cmp si, 22
jna mErr5Assert
test edi, 0x80000000  ; check dividend's sign bit (32-bit)
setnz r10b            ; save sign bit in r10b
mov eax, edi          ; rax (WQ) = dividend
neg edi               ; prepare negated value of dividend
mov ecx, [Base]       ; rcx = current number base
cmp cl, 10            ; if ((rdi<0) && ([Base] == 10)) { rax = abs(rdi) }
setz r11b             ; (this does i32 for decimal or u32 for hex)
and r10b, r11b
cmovnz rax, rdi       ; rax = dividend (for base 10, abs(dividend))
;/////////////////////
.forDigit:
                      ; Calculate value of least-significant digit (use Base)
dec rsi               ; decrement index into PadRtl
xor rdx, rdx          ; zero high register of dividend (may contain remainder)
idiv rcx              ; idiv args {[rdx:rax]: dividend, operand: divisor}
                      ; idiv result {rax: quotient, rdx: remainder}
;---------------------
                      ; Convert from number in 0..15 to {'0'..'9', 'A'..'F'}
mov r11b, dl          ; prepare ASCII digit as n-10+'A' (in case n in 10..15)
add r11b, ('A'-10)
add dl, '0'           ; prepare ASCII digit as n+'0' (in case n in 0..9)
cmp dl, '9'           ; rdx = pick the appropriate ASCII digit
cmova rdx, r11
mov [PadRtl+rsi], dl  ; store digit in rightmost unused byte
test rax, rax         ; stop if quotient was zero
jz .done
test rsi, rsi         ; stop if buffer is full
jz .doneError
jmp .forDigit         ; loop if buffer still has room for another digit
;/////////////////////
.done:
mov [PadRtl+StrMax], si  ; store updated index to PadRtl's leftmost used byte
test r10b, r10b       ; check if a '-' is needed
jnz .doneNegative
ret
;---------------------
.doneNegative:        ; add a '-' if base is decimal and sign is negative
jmp mFmtRtlMinus
;---------------------
.doneError:
mov W, 10             ; show error, assert(true, 10), if buffer got full. This
jmp mErr5Assert       ;   should not happen unless there is a logic error


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
cmp DSDeep, 0         ; if stack is empty, print the empty message
je .doneEmpty
call mFmtRtlClear     ; otherwise, prepare the formatting buffer
;---------------------
xor ebp, ebp          ; start index at 1 because of T
inc ebp
mov edi, T            ; prepare for mFmtRtlInt32(edi: T)
;---------------------
.for:                 ; Format stack cells
call mFmtRtlInt32     ; format(edi: current stack cell) into PadRtl
call mFmtRtlSpace     ; add a space (rtl, so space goes to left of number)
inc ebp               ; inc index
cmp ebp, DSDeep       ; stop if all stack cells have been formatted
ja .done
mov W, DSDeep         ; otherwise, prepare for mFmtRtlInt32(rdi: stack cell)
sub W, ebp            ; load next cell
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


;-----------------------------
; Dictionary: Misc

mBye:                         ; Set VM's bye flag to true
or VMFlags, VMBye
ret

;-----------------------------
; Dictionary: Host API for IO

mStrPut:               ; Write string [Pad] to stdout, clear [Pad]
lea WQ, [Pad]
.W:                    ; Write string [W] to stdout, clear [W]
mov rdi, WQ            ; *buf (note: W is eax, so save it first)
movzx esi, word [rdi]  ; count (string length is first word of string record)
add edi, 2             ; string data area starts at third byte of Pad
.RdiRsi:               ; Do mkb_host_write(rdi: *buf, rsi: count)
push rbp               ; align stack to 16 bytes
mov rbp, rsp
and rsp, -16
call mkb_host_write    ; call host api to write string to stdout
mov rsp, rbp           ; restore stack to previous alignment
pop rbp
ret
