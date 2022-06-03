; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth is a Forth system for archival-grade multi-media computing.
; [note: just getting started here, so this is still a bit aspirational]
;

%include "libmarkab/common_macros.nasm"
%include "libmarkab/generated_macros.nasm"

global markab_cold

extern mkb_host_write
extern mkb_host_step_stdin
extern mkb_host_TIB
extern mkb_host_TIB_LEN

extern mErr7UnknownWord

global Mem
global DSBase
global RSBase

global datVoc0Head
global datForthP
global datContext
global datDotS
global datDotSNone
global datCallDP
global datDP
global datLast
global datHeap
global datHeapEnd
global datDPStr

global mStrPut.W
global mStrPut.RdiRsi


;=============================
section .data
;=============================

;-----------------------------
; Codegen Import
;
; This includes:
; 1. `%define t...` defines for VM instruction token values (e.g. tReturn)
; 2. Jump table including:
;    - JumpTable: label for start of jump table
;    - JumpTableLen: define for length of jump table (to check valid tokens)
; 3. Data structure for vocabulary 0 (built-in words):
;    - Voc0Head: head of vocabulary 0 linked list
;
%include "libmarkab/generated_data.nasm"


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
;
; See libmarkab_macros.nasm for mkStr macro
;

align 16, db 0
db "== VM Strings =="

datVersion:  mkStr `Markab v0.0.1\ntype 'bye' or ^C to exit\n`
datDotSNone: mkStr "  Stack is empty"
datOK:       mkStr `  OK\n`
datVoc0Head: mkStr "[Voc0Head]  "
datForthP:   mkStr "ForthP     "
datContext:  mkStr "Context    "
datDP:       mkStr "DP         "
datLast:     mkStr "Last       "
datCallDP    mkStr "CallDP     "
datHeap:     mkStr "Heap       "
datHeapEnd:  mkStr "HeapEnd   "
datDPStr:    mkStr "str([DP])  "
datDotS:     mkStr ".s        "


;=============================
section .bss
;=============================


;-----------------------------
; Virtual CPU Stacks
;
; See libmarkab_macros.nasm for DSMax and RSMax
;

dwordBuf DSBase, DSMax-1      ; Data stack (size excludes top item T)
dwordBuf RSBase, RSMax-1      ; Return stack (size excludes top item R)


;-----------------------------
; Virtual Machine RAM
;
; See libmarkab_macros.nasm for memory layout
;

byteBuf Mem, MemSize          ; Buffer for virtual Memory area



;=============================
section .text
;=============================

extern mErr24CoreVocabTooLong
extern mErr16ScreenTooLong
extern mErr3BadToken
extern mErr3BadToken
extern mErr19BadAddress
extern mErr22LoopTooLong
extern mErr18ExpectedSemiColon
extern mErr28DPOutOfRange
extern mErr29BadVocabLink
extern mErr14BadWordType
extern mErr23BadBufferPointer
extern mErr23BadBufferPointer
extern mErr23BadBufferPointer
extern mErr8NoParen
extern mErr8NoParen
extern mWord
extern mPush
extern mLowercase
extern mPopW
extern mCompileU8
extern mCompileU16
extern mCompileLiteral
extern mNumber


;-----------------------------
; Library init entry point

markab_cold:
enter 0, 0
xor W, W                        ; init virtual registers and stacks
mov T, W
mov R, W
movq DSDeep, WQ
movq RSDeep, WQ
mov [Mem+Pad], WW               ; Zero string buffer length fields
mov [Mem+Blk], WW
mov [Mem+TIB], WW
mov [Mem+CallDP], word Heap     ; last compiled call pointer starts at 0
call mDecimal                   ; default number base
xor VMFlags, VMFlags            ; clear VM flags
lea W, datVersion               ; print version string
call mStrPut.W
;-----------------------------
mov W, [Voc0Head]             ; Initialize core vocabulary head pointers
mov word [Mem+ForthP], WW     ; point ForthP to core vocab
mov word [Mem+CompileP], WW   ;  same for CompileP (TODO: split core vocab)
mov WW, Last                  ; load address of pointer to head of dictionary
mov word [Mem+Context], WW    ; Set [Context] to address of Last
;-----------------------------
xor W, W
mov dword [Mem+Heap], W       ; Zero the first item of ExtV (TODO:rename ExtV?)
mov W, [Voc0Head]             ;  then set its link to Voc0Head
mov word [Mem+Heap], WW
mov WW, Heap                  ; Set [Last] to address of head item in ExtV
mov word [Mem+Last], WW
add W, 3                      ; Allot 3 bytes for ExtV's head item
mov word [Mem+DP], WW         ; Set [DP] to first available byte at end of ExtV
;-----------------------------
lea esi, [Voc0]               ; Copy core vocabulary to [Mem+CoreV]
lea edi, [Mem+CoreV]
mov ecx, [Voc0Len]
cmp ecx, CoreVSize
jnb mErr24CoreVocabTooLong
xor edx, edx                  ; Zero buffer index
.forCopyCoreV:                ; Copy the bytes
mov WB, [esi+edx]
mov [edi+edx], WB
inc edx                       ; Advance buffer index
dec ecx                       ; Decrement loop counter
jnz .forCopyCoreV
;-----------------------------
mov esi, Screen0              ; Copy screen0 to Block buffer
lea edi, [Mem+Blk]
mov ecx, [Screen0Len]
cmp ecx, BlkSize              ; stop if screen won't fit in Block
jnb mErr16ScreenTooLong
test ecx, ecx                 ; skip the copy if screen0 is empty
jz .skipCopyScreen0
mov [Mem+IBPtr], word Blk     ; Configure input buffer source as Block
mov [Mem+IBLen], cx
mov [Mem+IN], word 0
xor edx, edx                  ; Zero buffer index
.forCopyScreen0:              ; Copy the bytes from screen0 to Block
mov WB, [esi+edx]
mov [edi+edx], WB
inc edx                       ; Advance buffer index
dec ecx                       ; Decrement loop counter
jnz .forCopyScreen0
.skipCopyScreen0:
;-----------------------------
call markab_outer             ; Interpret loadscreen
mov [Mem+IBPtr], word TIB     ; Configure input buffer source as Terminal
mov [Mem+IBLen], word 0
mov [Mem+IN], word 0
;/////////////////////////////
.OuterLoop:                   ; Begin outer loop
test VMFlags, VMBye           ; Break loop if bye flag is set
jnz .done
movq r10, DSDeep              ; Save SSE registers
push r10
movq r11, RSDeep
push r11
push rbp                      ; align stack to 16 bytes
mov rbp, rsp
and rsp, -16
call mkb_host_step_stdin      ; step the non-blocking stdin state machine
mov rsp, rbp                  ; restore stack to previous alignment
pop rbp
pop r11                       ; Restore SSE registers
movq RSDeep, r11
pop r10
movq DSDeep, r10
cmp al, 0                     ; check return value
je .OuterLoop                 ; result = 0: keep looping
jl .done                      ; result < 0: exit interpreter
;-----------------------------
                              ; result > 0: interpret a line of text from TIB
lea esi, [mkb_host_TIB]
lea edi, [Mem+TIB]
mov ecx, [mkb_host_TIB_LEN]
mov W, TIBSize                  ; Clip input length at TIBSize
cmp ecx, TIBSize
cmova ecx, W
test ecx, ecx                   ; check if buffer is empty (maybe CR)
jz .skipCopyTIB
mov word [Mem+IBPtr], word TIB  ; Configure input bufer as TIB
mov word [Mem+IBLen], cx        ; Store input length to [IBLen]
mov word [Mem+IN], word 0       ; Zero [IN]
xor edx, edx                    ; Zero buffer index
.forCopyHostToTIB:              ; Copy bytes from host-side to TIB
mov WB, [esi+edx]
mov [edi+edx], WB
inc edx                       ; Advance buffer index
dec ecx                       ; Decrement loop limit
jnz .forCopyHostToTIB
xor eax, eax                  ; clear the host-side input buffer
mov [mkb_host_TIB_LEN], eax
.skipCopyTIB:
;-----------------------------
call markab_outer             ; call outer interpreter
jmp .OuterLoop                ; keep looping
;/////////////////////////////
.done:
leave
ret

;-----------------------------
; Interpreters

doTokenW:                     ; Run handler for one token [W: token]
cmp W, JumpTableLen           ; detect token beyond jump table range (CAUTION!)
jae mErr3BadToken
lea edi, [JumpTable]          ; fetch jump table address
mov esi, dword [edi+4*W]
jmp rsi                       ; jump


doInner:                      ; Inner interpreter (edi: virtual code pointer)
push rbp
push rbx
cmp edi, MemSize              ; check address satisfies: 0 <= edi < MemSize
jz .doneBadAddress            ; stop if address is out of range
mov ebp, edi                  ; ebp: instruction pointer (I)
mov ebx, 0x7ffff              ; ebx: max loop iterations (very arbitrary)
;/////////////////////////////
.for:
test ebp, ebp                 ; be sure that 0 <= ebp <= MemSize-1
jl .doneBadAddress
cmp ebp, MemSize-1
jg .doneBadAddress
movzx W, byte [Mem+ebp]       ; load token at I
cmp WB, JumpTableLen          ; detect token beyond jump table range (CAUTION!)
jae .doneBadToken
lea edi, [JumpTable]          ; fetch jump table address
mov esi, dword [edi+4*W]
inc ebp                       ; advance I
call rsi                      ; jump (callee may adjust I for LITx)
test VMFlags, VMErr           ; stop if token had an error
jnz .doneErr
test VMFlags, VMReturn        ; stop if token was an ending return
jnz .done
dec ebx                       ; stop if loop limit has been reached
jz .doneLoopLimit
jmp .for                      ; keep looping if checks passed
;/////////////////////////////
.done:                        ; normal exit path
and VMFlags, (~VMReturn)      ; clear VMReturn flag
pop rbx
pop rbp
ret
;-----------------------------
.doneErr:                     ; exit path for error (leave VMErr flag set!)
pop rbx
pop rbp
ret
;-----------------------------
.doneBadToken:                ; exit path for invalid token
call mErr3BadToken
pop rbx
pop rbp
ret
;-----------------------------
.doneBadAddress:              ; exit path for bad jump or return address
call mErr19BadAddress
pop rbx
pop rbp
ret
;-----------------------------
.doneLoopLimit:               ; exit when loop iterations passed limit
call mErr22LoopTooLong
pop rbx
pop rbp
ret

; Interpret a line of text from the input source buffer (via IBPtr)
markab_outer:
push rbp
;/////////////////////////////
.forNextWord:
movzx edi, word [Mem+IBPtr]   ; Fetch input buffer pointer (index to Mem)
cmp edi, BuffersStart         ; Stop if buffer pointer is out of range
jb .doneErr
cmp edi, BuffersEnd
jnb .doneErr
lea edi, [Mem+edi]            ; Resolve buffer pointer to address
movzx esi, word [Mem+IBLen]   ; Load and range check buffer's available bytes
cmp esi, _1KB
jnb .doneErr
;-----------------------------
movzx W, word [Mem+IN]        ; stop if there are no more bytes left in buffer
cmp W, esi
jnb .done
movzx ebp, word [Mem+DP]      ; save old DP (where word will get copied to)
call mWord                    ; copy a word from [IBPtr]+[IN] to [DP]
mov word [Mem+DP], bp         ; put DP back where it was
test VMFlags, VMErr           ; stop if error while copying word
jnz .doneErr
;-----------------------------
call doWord                   ; doWord expects [DP]: {db length, <word>}
test VMFlags, VMErr           ; stop if error while running word
jnz .doneErr
test VMFlags, VMBye           ; continue unless bye flag was set
jz .forNextWord
;/////////////////////////////
.done:                        ; Print OK for success (unless still compiling)
test VMFlags, VMCompile       ; make sure compile mode is not still active
jnz .doneStillCompiling
lea W, [datOK]
pop rbp
jmp mStrPut.W
;-----------------------------
.doneStillCompiling:
                              ; TODO: is this leaking heap for code???
and VMFlags, (~VMCompile)     ; clear compiling flag
;...                          ; roll back to before the unfinished define
movzx edi, word [Mem+Last]    ;  load unfinished dictionary entry
;...                          ; TODO: maybe validate this link address ???
movzx esi, word [Mem+edi]     ;  rsi = {dd .link} (link to last valid entry)
mov word [Mem+Last], si       ;  roll back the dictionary head
call mErr18ExpectedSemiColon  ; print errror
and VMFlags, (~VMErr)         ; clear error bit
pop rbp
jmp mCR                       ; print CR without OK
;-----------------------------
.doneErr:                     ; Print CR (skip OK), clear error bit
test VMFlags, VMCompile
jz .doneErrSkip
and VMFlags, (~VMCompile)     ; clear compiling flag
;...                          ; roll back to before the unfinished define
movzx edi, word [Mem+Last]    ;  load unfinished dictionary entry
;...                          ; TODO: maybe validate this link address ???
movzx esi, word [Mem+edi]     ;  rsi = {dd .link} (link to last valid entry)
mov word [Mem+Last], si       ;  roll back the dictionary head
.doneErrSkip:
and VMFlags, (~VMErr)         ; clear error bit
pop rbp
jmp mCR                       ; print CR without the OK


; Look up word at [DP] in vocabulary pointed to by [Context]
;
; Results
; - match:    push {addr true} to stack (addr = byte after matching name)
; - no match: push     {false} to stack
; - error:    no stack changes, call error handler (which will set VMErr)
;
mFind:
push rbp
push rbx
fPush DP,        .err         ; -> {T: DP (pointer dictionary end + 1)}
fDo   WordFetch, .errDP       ; -> {T: address of string length count}
fDo   Dup,       .err         ; -> {S: addr, T: addr}
fDo   ByteFetch, .errDP       ; -> {S: addr, T: string count}
mov ebx, T                    ; ebx: count (save for later)
fDo   Drop,      .err         ; -> {T: address of count}
fDo   OnePlus,   .err         ; -> {T: address of string buffer}
lea ebp, [Mem+T]              ; ebp: *buf (actual address, save for later)
mov edi, ebp                  ; edi: *buf
mov esi, ebx                  ; esi: count
call mLowercase               ; Lowercase word: lowercase(edi:*buf, esi:count)
fDo   Drop,      .err         ; -> {}
;-----------------------------
fPush Context,   .err         ; -> {T: pointer to pointer to vocab head}
fDo   WordFetch, .errVoc      ; -> {T: pointer to vocab head}
fDo   WordFetch, .errVoc      ; -> {T: address of vocab item (head item)}
mov esi, T                    ; esi: address of list item
push rsi
call mDrop                    ; -> {}
pop rsi
;-----------------------------
.compareLength:               ; Check if lengths match
xor W, W
mov WB, byte [Mem+esi+2]      ; WB: length of dictionary item name field
cmp WB, bl                    ; compare it to search word's length (count)
jnz .nextItem                 ;  if no match, follow link to next item
;-----------------------------
.compareSpelling:             ; Length matched, so now compare spelling
movzx ecx, bl                 ; ecx: loop limit counter (length of word)
lea edi, [Mem+esi+3]          ; edi: base address of list item name field
xor W, W                      ; W: buffer index
;-----------------------------
.forSpelling:
mov dl, [ebp+W]               ; load byte of search word
cmp dl, [edi+W]               ; load byte of dictionary item name
jnz .nextItem                 ; break if bytes don't match
inc W                         ; keep looping if there are more bytes to check
dec rcx
jnz .forSpelling              ; no more bytes means search word matches name
;-----------------------------
.wordMatch:                   ; Mem+esi+3+W points to byte after name field
add W, esi                    ; W = index into Mem for byte after name
add W, 3
call mPush                    ; Push {W: match index} to the data stack
jmp .done
;-----------------------------
.nextItem:                    ; Follow link to next item (current is Mem+esi)
movzx esi, word [Mem+esi]     ; load current list item's link field
test esi, esi                 ; link address of zero marks end of list
jnz .compareLength            ; if more items: follow link
;------------------------
.doneNoMatch:                 ; Normal exit path when word did not match
fPush 0, .err                 ; push {W: false (0)} to the data stack
pop rbx
pop rbp
ret
;-----------------------------
.done:                        ; Normal exit path for match
fPush -1, .err                ; push {W: true (-1)} to the data stack
pop rbx
pop rbp
ret
;-----------------------------
.err:                         ; Exit path for already-reported error
pop rbx
pop rbp
ret
;-----------------------------
.errDP:                       ; Oops... virtual memory got corrupted
pop rbx
pop rbp
jmp mErr28DPOutOfRange
;-----------------------------
.errVoc:                      ; Oops... virtual memory got corrupted
pop rbx
pop rbp
jmp mErr29BadVocabLink


; Attempt to do the action for a word, potentially including:
;   1. Interpret it according to the dictionary
;   2. Push it to the stack as a number
;   3. Print an error message
; doWord expects [DP]: {db length, <word>}
doWord:
push rbp
push rbx
fPush DP,        .err        ; -> {T: DP (address of dictionary pointer)}
fDo   WordFetch, .err        ; -> {T: [DP] (address of string for word's name)}
fDo   ByteFetch, .err        ; -> {T: [[DP]] (length field of string)}
fDo   PopW,      .err        ; -> {}, {W: [[DP]]}
test W, W                    ; stop if length of word is 0
jz .done
;---------------------------
fDo Find,      .err          ; Look for word in vocab pointed to by Context
;...                         ; -> {S: address, T: -1 (true)} or {T: 0 (false)}
fDo PopW,      .err          ; popw -> {T: address} or {}, [W: true or false]
test W, W                    ; check for match
jz .wordNotFound             ; at this point, stack is {}
;---------------------------
.wordMatch:                  ; Decide what to do with {T: address of .type}
fDo Dup,       .err          ;  -> {S: .type, T: .type)}
fDo OnePlus,   .err          ;  -> {S: .type, T: .param = (.type+1)}
fDo Swap,      .err          ;  -> {S: .param, T: .type}
fDo ByteFetch, .err          ;  -> {S: .param, T: [.type]}
fDo PopW,      .err          ;  -> {T: .param}, {W: [.type]}
cmp WB, TpConst              ; if type const -> 32-bit param is constant
je .paramConst
cmp WB, TpVar                ; if type var   -> 32-bit param is variable
je .TpVar
cmp WB, TpCode               ; if type code  -> 16-bit param is code pointer
je .paramCode
cmp WB, TpToken              ; if type token -> 16-bit param is token:immediate
jne .errBadWordType
;---------------------------
.paramToken:                 ; Handle token; stack is {T: .param}
fDo WordFetch, .err          ; fetch -> {T: token:immediate}
test VMFlags, VMCompile      ; if compiling and word is non-immediate, branch
setnz r10b                   ;  set means compile mode active
test TW, 0xff00              ;  if (second-byte==0) -> token is non-immediate
setz r11b
and TW, ~0xff00              ; clear immediate indicator byte, leaving token
and r10b, r11b
jnz .compileToken            ; if compiling non-immediate word: jump
fDo PopW,      .err          ; -> {}, {W: token}
call doTokenW                ; run handler for the token in W
jmp .done
;---------------------------
.compileToken:               ; Compile token; stack is {T: token}
fDo  CompileU8,  .err
jmp .done
;---------------------------
.paramCode:                  ; Run compiled code-pointer {T: .param}
test VMFlags, VMCompile      ; if compile mode: branch
jnz .compileCode
fDo PopW,        .err        ; -> {}, {W: .param (address of compiled code)}
mov edi, W                   ; edi: .param  (address of code)
call doInner                 ; else: doInner(edi: .param)
jmp .done
;---------------------------
.compileCode:                ; Compile call to code pointer {T: .param}
;...                         ; Remember [DP] to help the tail call optimizer
;...                         ;  (see mSemiColon for how that works)
fDo   Here,       .err       ; -> {S: .param, T: [DP]}
fPush CallDP,     .err       ; -> {.param, S: [DP], T: CallDP}
fDo   WordStore,  .err       ; -> {T: .param}
fPush tCall,      .err       ; -> {S: .param, T: tCall}
fDo   CompileU8,  .err       ; -> {.param}   (compile tCall token)
fDo   CompileU16, .err       ; -> {}         (compile call address)
jmp .done
;---------------------------
.TpVar:                      ; Handle a variable {T: .param}
test VMFlags, VMCompile
jz .done                     ; if interpreting, {T: .param} is what we need
;-------------
.compileVar:
fDo CompileLiteral, .err     ; -> {}   (compile T as number literal to heap)
jmp .done
;---------------------------
.paramConst:                 ; Handle a constant {T: .param}
fDo   Fetch,     .err        ; -> {T: [.param] (32-bit value of the const)}
test VMFlags, VMCompile
jz .done
;-------------
.compileConst:
fDo CompileLiteral, .err     ; -> {}   (compile T as number literal to heap)
jmp .done
;---------------------------
.wordNotFound:
movzx ecx, word [Mem+DP]
lea edi, [Mem+ecx+1]        ; prepare args for mNumber(edi: *buf, esi: count)
movzx esi, byte [Mem+ecx]
mov rbp, rdi
mov rbx, rsi
call mNumber                ; attempt to convert word as number
test VMFlags, VMNaN
jnz .errNaN                 ; stop if word was not a number
test VMFlags, VMCompile
jz .done
;---------------------------
.compileNumber:
fDo CompileLiteral, .err    ; compile the number with an integer literal token
;---------------------------
.done:
pop rbx
pop rbp
ret
;---------------------
.err:
pop rbx
pop rbp
ret
;---------------------
.errNaN:                    ; Handle a word that didn't match anything
and VMFlags, (~VMNaN)       ; clear the NaN flag and show an error
movzx ecx, word [Mem+Base]
cmp cl, 16
call mErr7UnknownWord       ; print NaN error message prefix,
mov rdi, rbp                ;  then print the word that wasn't found
mov rsi, rbx
call mStrPut.RdiRsi
or VMFlags, VMErr           ; return with error condition
pop rbx
pop rbp
ret
;---------------------
.errBadWordType:
pop rbx
pop rbp
jmp mErr14BadWordType


; Paren comment: skip input text until ")"
;   input bytes come from [IBPtr] using [IBLen] and [IN]
mParen:
movzx edi, word [Mem+IBPtr]   ; Fetch input buffer pointer (index to Mem)
cmp edi, BuffersStart         ; Stop if buffer pointer is out of range
jb mErr23BadBufferPointer
cmp edi, BuffersEnd
jnb mErr23BadBufferPointer
lea edi, [Mem+edi]            ; Resolve buffer pointer to address
movzx esi, word [Mem+IBLen]   ; Load and range check buffer's available bytes
cmp esi, _1KB
jnb mErr23BadBufferPointer
movzx ecx, word [Mem+IN]      ; ecx = IN (index to next available byte of TIB)
cmp esi, ecx                  ; stop looking if TIB_LEN <= IN (ran out of bytes)
jna mErr8NoParen
;------------------------
.forScanParen:
mov WB, [edi+ecx]        ; check if current byte is ')'
cmp WB, ')'
jz .done                 ; if so, stop looking
inc ecx                  ; otherwise, continue with next byte
cmp esi, ecx             ; loop until index >= TIB_LEN
ja .forScanParen
jmp mErr8NoParen         ; oops, end of TIB reached without finding ")"
;------------------------
.done:
inc ecx
mov word [Mem+IN], cx
ret

mTick:                    ; TICK (') - Push address of word from input bufer
push rbp
movzx ebp, word [Mem+DP]  ; save DP since mWord will move it
fDo Word, .err            ; copy next word of input buffer to [DP]
mov word [Mem+DP], bp     ; restore DP so it points to start of string
fDo Find, .err            ; find address of word named by string at [DP]
;...                      ;  -> {S: address, T: -1 (true)} or {T: 0 (false)}
test T, T                 ; check if it worked
jnz .match
;-------------------------
.noMatch:                 ; Exit path for string not found -> {T: 0 (false)}
pop rbp
ret
;-------------------------
.match:                   ; Exit for match: return pointer to .param field
call mDrop                ; -> {T: address of .param}
pop rbp
ret
;-------------------------
.err:                     ; Exit path for error; stack state is undefined!
pop rbp
ret


;-----------------------------
; Dictionary: Misc

mBye:                         ; Set VM's bye flag to true
or VMFlags, VMBye
ret

;-----------------------------
; Dictionary: Host API for IO

mStrPut:               ; Write string [Pad] to stdout, clear [Pad]
lea WQ, [Mem+Pad]
.W:                    ; Write string [W] to stdout, clear [W]
mov rdi, WQ            ; *buf (note: W is eax, so save it first)
movzx esi, word [rdi]  ; count (string length is first word of string record)
add edi, 2             ; string data area starts at third byte of Pad
.RdiRsi:               ; Do mkb_host_write(rdi: *buf, rsi: count)
movq r10, DSDeep       ; Save SSE registers
push r10
movq r11, RSDeep
push r11
push rbp               ; align stack to 16 bytes
mov rbp, rsp
and rsp, -16
call mkb_host_write    ; call host api to write string to stdout
mov rsp, rbp           ; restore stack to previous alignment
pop rbp
pop r11                ; Restore SSE registers
movq RSDeep, r11
pop r10
movq DSDeep, r10
ret
