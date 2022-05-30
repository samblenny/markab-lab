; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth is a Forth system for archival-grade multi-media computing.
; [note: just getting started here, so this is still a bit aspirational]
;

bits 64
default rel
global markab_cold

extern mkb_host_write
extern mkb_host_step_stdin
extern mkb_host_TIB
extern mkb_host_TIB_LEN


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
datErr2sf:   mkStr "  E2 Stack overflow (cleared stack)"
datErr3bt:   mkStr "  E3 Bad token: "
datErr4nq:   mkStr `  E4 Expected \"`
datErr5nf:   mkStr "  E5 Number format"
datErr6of:   mkStr "  E6 Overflow: "
datErr7nfd:  mkStr "  E7 ? "
datErr7nfh:  mkStr "  E7 [hex] ? "
datErr8np:   mkStr "  E8 Expected )"
datErr9df:   mkStr "  E9 Dictionary full"
datErr10en:  mkStr "  E10 Expected name"
datErr11ntl: mkStr "  E11 Name too long"
datErr12dbz: mkStr "  E12 Divide by 0"
datErr13aor: mkStr "  E13 Address out of range"
datErr14bwt: mkStr "  E14 Bad word type"
datErr15hf:  mkStr "  E15 Heap full"
datErr16stl: mkStr "  E16 Screen too long"
datErr17snc: mkStr "  E17 ; when not compiling"
datErr18esc: mkStr "  E18 Expected ;"
datErr19ba:  mkStr "  E19 Bad address"
datErr20rsu: mkStr "  E20 Return stack underflow"
datErr21rsf: mkStr "  E21 Return stack full"
datErr22ltl: mkStr "  E22 Loop too long"
datErr23bbp: mkStr "  E23 Bad buffer pointer"
datErr24cvt: mkStr "  E24 Core vocab too long"
datErr25bor: mkStr "  E25 Base out of range"
datErr26fin: mkStr "  E26 Format insert"
datErr27bfi: mkStr "  E27 Bad format index"
datErr28dpo: mkStr "  E28 DP out of range"
datErr29bvl: mkStr "  E29 Bad vocab link"
datDotSNone: mkStr "  Stack is empty"
datOK:       mkStr `  OK\n`
datVoc0Head: mkStr "[Voc0Head]  "
datForthP:   mkStr "ForthP     "
datExtV:     mkStr "ExtV       "
datContext:  mkStr "Context    "
datDP:       mkStr "DP         "
datLast:     mkStr "Last       "
datCodeP:    mkStr "CodeP      "
datCodeCallP mkStr "CodeCallP  "
datHeap:     mkStr "Heap       "
datHeapEnd:  mkStr "HeapEnd   "
datDPStr:    mkStr "str([DP])  "
datDotS:     mkStr ".s        "

align 16, db 0
db "=== End.data ==="

;=============================
section .bss
;=============================

%macro byteBuf 2
  align 16, resb 0
  %1: resb %2
%endmacro
%macro dwordBuf 2
  align 16, resb 0
  %1: resd %2
%endmacro
%macro dwordVar 1
  align 4, resb 0
  %1: resd 1
%endmacro

;-----------------------------
; Virtual CPU Stacks

%define DSMax 17              ; total size of data stack (T + 16 dwords)
%define RSMax 17              ; total size of return stack (R + 16 dwords)
dwordBuf DSBase, DSMax-1      ; Data stack (size excludes top item T)
dwordBuf RSBase, RSMax-1      ; Return stack (size excludes top item R)


;-----------------------------
; Virtual Machine RAM Layout
;
; Glossary
; - Vocabulary: Set of words in the form of a linked list
; - Dictionary: Set of all the vocabularies
; - Virtual RAM: Zero-indexed address space available to ! and @
;
%define _1KB 1024
%define _2KB 2048
%define _4KB 4096
%define _8KB 8192
%define _16KB 16384
%define _32KB 32768
%define _64KB 65536

%define MemSize _64KB            ; Size of virtual Memory area
byteBuf Mem, MemSize             ; Buffer for virtual Memory area

;--- Built-in Vocabularies (write protected area in Mem for builtins) ---
;
%define CoreV 0                  ; Start index of Core Vocabularies area
%define CoreVSize _1KB           ; Size of Core Vocabularies area
%define CoreVEnd CoreVSize-1     ; End index of Core Vocabularies area

%define ForthP CoreVEnd+2        ; dw Pointer to head of Forth vocabulary
%define CompileP ForthP+2        ; dw Pointer to head of Compile vocabulary
%define Context CompileP+2       ; dw Pointer to current selected vocabulary

%define BuiltinEnd Context+2-1   ; End of builtins area

%define Fence BuiltinEnd         ; Index of write protect boundary

;--- Extensible Vocabulary (read/write area in Mem for adding new words) ---
;
%define DP Fence+1               ; dw Pointer to next free byte of Ext. Vocab
%define Last DP+2                ; dw Pointer to head of Ext. Vocab

%define ExtV Last+2              ; Start index of extensible vocabulary area
%define ExtVSize _4KB            ; Size of extensible vocabulary area
%define ExtVEnd ExtV+ExtVSize-1  ; End index of extensible vocabulary area

;--- Buffers (scratch pad, terminal input, formatting, ...) ---
;
%define BuffersStart ExtVEnd+1   ; Start of buffers

%define Pad BuffersStart         ; Start of scratchPad string buffer
%define PadSize _1KB             ; Size of scratchPad
%define PadEnd Pad+PadSize-1     ; End of scratchPad

%define Fmt PadEnd+1             ; Start of right-to-left Formatting buffer
%define FmtSize _1KB             ; Size of Fmt
%define FmtEnd Fmt+FmtSize-1     ; End offset of Fmt buffer within Mem
%define FmtLI FmtEnd+1           ; dw Fmt buffer Left Index (leftmost byte)
%define FmtSgn FmtLI+2           ; db Sign of number to be formatted
%define FmtQuo FmtSgn+1          ; dd Quotient i32 (temp var for formatting)
%define FmtRem FmtQuo+4          ; dd Remainder i32 (temp var for formatting)

%define Blk FmtRem+4             ; Start of Block buffer
%define BlkSize _1KB             ; Size of Block buffer
%define BlkEnd Blk+BlkSize-1     ; End of Block buffer

%define TIB BlkEnd+1             ; Start of Terminal Input Buffer (TIB)
%define TIBSize _1KB             ; Size of TIB
%define TIBEnd TIB+TIBSize-1     ; End of TIB

%define IBPtr TIBEnd+1           ; dw Input Buffer Pointer (IBP)
%define IBLen IBPtr+2            ; dw Length of data in IBP (bytes available)
%define IN IBLen+2               ; dw Index into IBP for next unparsed byte

%define Base IN+2                ; dw Number base for number/string conversions

%define EmitBuf Base+2           ; Start of buffer for emit to use
%define EmitBufEnd EmitBuf+8     ; End of buffer for emit

%define BuffersEnd EmitBufEnd    ; End of string buffers area

;--- Heap memory for variables and compiled code ---
;
%define CodeP BuffersEnd+1       ; dw Pointer to next free byte of heap memory
%define CodeCallP CodeP+2        ; dw Pointer to last compiled call token

%define Heap CodeCallP+2         ; Start of heap area
%define HReserve 260             ; Heap reserved bytes (space for mWord, etc)
%define HeapEnd MemSize-1        ; End of heap area



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
%define VMCompile 8           ; Compile bit: set means compile mode is active
%define VMReturn 16           ; Return bit: set means end of outermost word
%define VMFlags r12b          ; Virtual machine status flags

%define T r13d                ; Top item of data stack (32-bits)
%define TW r13w               ; Top item of data stack (low word)
%define TB r13b               ; Top item of data stack (low byte)
%define R r15d                ; Return stack top item (32-bits)
%define DSDeep xmm0           ; Depth of data stack (includes T, NOTE! XMM0)
%define RSDeep xmm1           ; Depth of return stack (NOTE! XMM1 register)


;-----------------------------
; Pseudo-Forth macros
;
; These are for writing assembly subroutines by chaining together calls to the
; subroutines for VM instruction tokens. It's like unrolling the behavior of
; the inner interpreter. The last argument to all of these macros is a label
; to use as the jump target if there's a VM error in the token handler.
;
; This is an intermediate step towards writing features in Forth instead of
; assembly. For now, as I'm debugging the plumbing around dictionary stuff, I
; need a thing shaped like this so I can stop spending so much time and effort
; on boilerplate for working with pointers to virtual memory addresses.
;
%macro fPush 2
 mov W, %1
 call mPush
 test VMFlags, VMErr
 jnz %2
%endmacro
%macro fDo 2
 call m%[%1]
 test VMFlags, VMErr
 jnz %2
%endmacro


;-----------------------------
; Error messages

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
mov [Mem+CodeP], word Heap      ; code memory pointer starts at Heap[0]
mov [Mem+CodeCallP], word Heap  ; last compiled call pointer starts at 0
call mDecimal                   ; default number base
xor VMFlags, VMFlags            ; clear VM flags
lea W, datVersion               ; print version string
call mStrPut.W
;-----------------------------
mov W, [Voc0Head]             ; Initialize core vocabulary head pointers
mov word [Mem+ForthP], WW     ; point ForthP to core vocab
mov word [Mem+CompileP], WW   ;  same for CompileP (TODO: split core vocab)
mov WW, Last                  ; load address of pointer to head of ExtV
mov word [Mem+Context], WW    ; Set [Context] to address of Last
;-----------------------------
xor W, W
mov dword [Mem+ExtV], W       ; Zero the first item of ExtV
mov W, [Voc0Head]             ;  then set its link to Voc0Head
mov word [Mem+ExtV], WW
mov W, ExtV                   ; Set [Last] to address of head item in ExtV
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
pop rbp
lea W, [datOK]
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
call mErr18ExpectedSemiColon  ; fall through to .doneErr
;-----------------------------
.doneErr:                     ; Print CR (skip OK), clear error bit
pop rbp
test VMFlags, VMCompile       ; make sure we're not still compiling
jz .doneErr_
call .doneStillCompiling
.doneErr_:
and VMFlags, (~VMErr)
jmp mCR


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
fPush ExtV,       .end1  ; ExtV         <address> <contents>
lea W, [datExtV]
call   .mDumpOneVar
fPush CodeP,      .end1  ; CodeP        <address> <contents>
lea W, [datCodeP]
call   .mDumpOneVar
fPush CodeCallP,  .end1  ; CodeCallP    <address> <contents>
lea W, [datCodeCallP]
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

mPrintDPStr:             ; Print string from [DP]
push rbp
fPush DP,        .end    ; fush  -> {T: DP (pointer to end of dictionary)}
fDo   WordFetch, .end    ; fetch -> {T: address (end of dictionary)}
fDo   Dup,       .end    ; copy  -> {S: addr, T: addr}
fDo   ByteFetch, .end    ; fetch -> {S: addr, T: string length count}
mov ebp, T               ; ebp: count
fDo   Drop,      .end    ; drop  -> {T: addr}
add T, 1
lea edi, [Mem+T]         ; edi: *buf
mov esi, ebp             ; esi: count
call mStrPut.RdiRsi      ; print the string at [DP]
fDo   Drop,      .end    ; drop  -> {}
.end:
pop rbp
ret


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
fPush DP,        .err         ; push  -> {T: DP (pointer dictionary end + 1)}
fDo   WordFetch, .errDP       ; fetch -> {T: address of string length count}
fDo   Dup,       .err         ; copy  -> {S: addr, T: addr}
fDo   ByteFetch, .errDP       ; fetch -> {S: addr, T: string count}
mov ebx, T                    ; ebx: count (save for later)
fDo   Drop,      .err         ; drop  -> {T: address of count}
add T, 1                      ; 1+    -> {T: address of string buffer}
lea ebp, [Mem+T]              ; ebp: *buf (actual address, save for later)
mov edi, ebp                  ; edi: *buf
mov esi, ebx                  ; esi: count
call mLowercase               ; Lowercase word: lowercase(edi:*buf, esi:count)
fDo   Drop,      .err         ; drop  -> {}
;-----------------------------
fPush Context,   .err         ; push  -> {T: pointer to pointer to vocab head}
fDo   WordFetch, .errVoc      ; fetch -> {T: pointer to vocab head}
fDo   WordFetch, .errVoc      ; fetch -> {T: address of vocab item (head item)}
mov esi, T                    ; esi: address of list item
push rsi
call mDrop                    ; drop  -> {}
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
fDo Find,      .err          ; Look for word in vocab pointed to by Context
;...                         ; -> {S: address, T: -1 (true)} or {T: 0 (false)}
fDo PopW,      .err          ; popw -> {T: address} or {}, [W: true or false]
test W, W                    ; check for match
jz .wordNotFound             ; at this point, stack is {}
;---------------------------
.wordMatch:                  ; Decide what to do with {T: address of .type}
fDo Dup,       .err          ;  -> {S: .type, T: .type)}
inc T                        ;  -> {S: .type, T: .param = (.type+1)}
fDo Swap,      .err          ;  -> {S: .param, T: .type}
fDo ByteFetch, .err          ;  -> {S: .param, T: [.type]}
fDo PopW,      .err          ;  -> {T: .param}, {W: [.type]}
cmp WB, ParamConst           ; if type const -> 32-bit param is constant
je .paramConst
cmp WB, ParamVar             ; if type var   -> 32-bit param is variable
je .paramVar
cmp WB, ParamCode            ; if type code  -> 16-bit param is code pointer
je .paramCode
cmp WB, ParamToken           ; if type token -> 16-bit param is token:immediate
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
movzx ecx, word [Mem+CodeP]
cmp ecx, HeapEnd-HReserve    ; make sure heap has room
jnb .errHeapFull
mov [Mem+rcx], TB            ; store the token
inc ecx                      ; advance the code pointer
mov [Mem+CodeP], cx
fDo Drop,      .err          ; drop -> {}
jmp .done
;---------------------------
.paramCode:                  ; Run compiled code-pointer {T: .param}
fDo WordFetch, .err          ; -> {T: [.param] = *code (token code pointer)}
test VMFlags, VMCompile      ; if compile mode: branch
jnz .compileCode
fDo PopW,      .err          ; -> {}, {W: *code}
mov edi, W                   ; edi: *code  (virtual code pointer)
call doInner                 ; else: doInner(edi: virtual code pointer)
jmp .done
;---------------------------
.compileCode:                ; Compile call to code pointer {T: *code}
;...                         ; Fetch the address of first free heap byte
fPush CodeP,     .err        ;  -> {S: *code, T: address of CodeP}
fDo   WordFetch, .err        ;  -> {S: *code, T: value of [CodeP]}
cmp T, HeapEnd-HReserve      ; Check if heap has space for more code
jnb .errHeapFull             ;  r10b set means heap is full
;...                         ; Remember [CodeP] to help the tail call optimizer
;...                         ;  (see mSemiColon for how that works)
fDo   Dup,       .err        ;  -> {*code, S: [CodeP], T: [CodeP]}
fPush CodeCallP, .err        ;  -> {*code, [CodeP], [CodeP], T: CodeCallP}
fDo   WordStore, .err        ;  -> {S: *code, T: [CodeP]}
;...                         ; Store the Call token at end of heap
fPush tCall,     .err        ;  -> {*code, S: [CodeP], T: tCall}
fDo   Over,      .err        ;  -> {*code, [CodeP], S: tCall, T: [CodeP]}
fDo   ByteStore, .err        ;  -> {S: *code, T: [CodeP]}
inc T                        ;  -> {S: *code, T: [CodeP]+1}
;...                         ; Store the call address at [CodeP]+1
fDo   Swap,      .err        ;  -> {S: [CodeP]+1, T: *code}
fDo   Over,      .err        ;  -> {[CodeP]+1, S: *code, T: [CodeP+1]}
fDo   WordStore, .err        ;  -> {T: [CodeP]+1}
add T, 2                     ;  -> {T: [CodeP]+3}
;...                         ; Update CodeP with new first free heap byte
fPush CodeP,     .err        ;  -> {S: [CodeP]+3, T: address of CodeP}
fDo   WordStore, .err        ;  -> {}
jmp .done
;---------------------------
.paramVar:                   ; Handle a variable {T: .param}
test VMFlags, VMCompile
jz .done                     ; if interpreting, {T: .param} is what we need
;-------------
.compileVar:
; TODO: Decide, what does VARIABLE mean when compiling? Is this an error?
fDo Drop,      .err          ; drop -> {}
jmp .done
;---------------------------
.paramConst:                 ; Handle a constant {T: .param}
test VMFlags, VMCompile
jnz .compileConst
fDo   Fetch,     .err        ; -> {T: [.param] (32-bit value of the const)}
jmp .done
;-------------
.compileConst:
; TODO: Decide, what does CONSTANT mean when compiling? Is this an error?
fDo Drop,      .err          ; drop -> {}
jmp .done
;---------------------------
.wordNotFound:
movzx ecx, word [Mem+DP]
lea edi, [Mem+ecx+1]        ; prepare args for mNumber(edi: *buf, esi: count)
movzx esi, byte [Mem+ecx]
mov rbp, rdi
mov rbx, rsi
call mNumber                ; attempt to convert word as number
test VMFlags, VMNaN         ; check if it worked
jz .done
and VMFlags, (~VMNaN)       ; ...if not, clear the NaN flag and show an error
lea W, [datErr7nfd]         ; not found error message (decimal version)
lea edx, [datErr7nfh]       ; swap error message for hex version if base 16
movzx ecx, word [Mem+Base]
cmp cl, 16
cmove W, edx
call mStrPut.W              ; print the not found error prefix
mov rdi, rbp                ; print the word that wasn't found
mov rsi, rbx
call mStrPut.RdiRsi
or VMFlags, VMErr           ; return with error condition
;/////////////////////
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
.errBadWordType:
pop rbx
pop rbp
jmp mErr14BadWordType
;---------------------
.errHeapFull:         ; stack starts as {S: *code, T: value of [CodeP]}
call mDrop            ; drop -> {T: *code}
call mDrop            ; drop -> {}
pop rbx
pop rbp
jmp mErr15HeapFull
;---------------------
.errBadAddress:
pop rbx
pop rbp
jmp mErr19BadAddress
;/////////////////////


;-----------------------------
; Dictionary: Literals
;
; These are designed for efficient compiling of tokens to push signed 32-bit
; numeric literals onto the stack for use with signed 32-bit math operations.
;

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
test esi, esi            ; stop now if string is empty (optimize it out)
jz .done
movzx ecx, word [Mem+CodeP]  ; check if code memory has space
cmp ecx, HeapEnd-HReserve
jnb mErr15HeapFull
mov r10, rdi             ; r10 = save source string pointer
mov r11, rsi             ; r11 = save source string byte count
mov WB, tDotQuoteC       ; Store token for compiled version of ." at [CodeP]
mov byte [Mem+ecx], WB
inc ecx                  ; advance CodeP
mov word [Mem+ecx], si   ; store string length (TODO: integer overflow?)
add ecx, 2               ; advance CodeP
;------------------------
mov r9, r11              ; loop limit counter = saved source string length
mov rsi, r10             ; rsi = saved source string pointer
xor r8d, r8d             ; zero source index
xor W, W
.forCopy:
mov WB, byte [esi+r8d]   ; load source byte from TIB
mov byte [Mem+ecx], WB   ; store dest byte in CodeMem
inc ecx                  ; advance source index (CodeP)
inc r8d                  ; advance destination index (0..source_lenth-1)
dec r9d                  ; check loop limit
jnz .forCopy
mov word [Mem+CodeP], cx  ; update [CodeP]
;------------------------
.done:
ret
;------------------------
.doneErr:
jmp mErr23BadBufferPointer

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
; Compiling words

mColon:                       ; COLON - define a word
movzx edi, word [Mem+DP]      ; load dictionary pointer [DP]
push rdi                      ; save [DP] in case rollback needed
call mCreate                  ; add name from input stream to dictionary
test VMFlags, VMErr           ; stop and roll back dictionary if it failed
jnz .doneErr
;-----------------------------
movzx esi, word [Mem+DP]      ; load dictionary pointer (updated by create)
cmp esi, HeapEnd-HReserve     ; stop if there is not enough room to add a link
jnb .doneErrFull
mov [Mem+esi], byte ParamCode ; append {.wordType: code} (type is code pointer)
inc esi
movzx W, word [Mem+CodeP]     ; CAUTION! code pointer in dictionary is _word_
mov word [Mem+esi], WW        ; append {.param: dw [CodeP]} (the code pointer)
add esi, 2
mov word [Mem+DP], si
;-----------------------------
.done:
pop rdi                       ; commit dictionary changes
mov word [Mem+Last], di
or VMFlags, VMCompile         ; set compile mode
ret
;-----------------------------
.doneErrFull:
pop rdi                       ; roll back dictionary changes
mov word [Mem+DP], di
jmp mErr9DictFull             ; show error message
;-----------------------------
.doneErr:
pop rdi                       ; roll back dictionary changes
mov word [Mem+DP], di
ret

mSemiColon:                   ; SEMICOLON - end definition of a new word
test VMFlags, VMCompile       ; if not in compile mode, invoking ; is an error
jz mErr17SemiColon
movzx ecx, word [Mem+CodeP]
cmp ecx, Heap                 ; check that: Heap <= [CodeP] < HeapEnd-1
jb mErr19BadAddress
cmp ecx, HeapEnd-1
jnb mErr15HeapFull
;-----------------------------
.optimizerCheck:                ; Check if tail call optimization is possible
movzx r8, word [Mem+CodeCallP]  ; load address of last compiled call
mov W, ecx                      ; load address for current code pointer
sub W, 3                        ; check if offset is (token + _word_ address)
cmp r8d, W                      ;  zero means this `;` came right after a Call
je .rewriteTailCall
;------------------------------
.normalReturn:
xor rsi, rsi                    ; store a Return token in code memory
mov sil, tReturn
mov byte [Mem+ecx], sil
inc ecx                         ; advance the code pointer
mov word [Mem+CodeP], cx
xor W, W
mov word [Mem+CodeCallP], WW    ; clear the last compiled call pointer
and VMFlags, (~VMCompile)       ; clear the compile mode flag
ret
;-----------------------------
.rewriteTailCall:             ; Do a tail call optimization
mov WB, byte [Mem+r8d]        ; make sure last call is pointing to a tCall
cmp WB, tCall
jnz .normalReturn             ; if not: don't optimize
mov [Mem+r8d], byte tJump     ; else: rewrite the tCall to a tJump
and VMFlags, (~VMCompile)     ; clear the compile mode flag
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

mVariable:             ; VARIABLE -- create name, allot 4, update [Last]
fDo   Here,      .end  ; -> {T: [DP] (pointer to new dictionary entry)}
fDo   Create,    .end  ; Read name from input stream and add it to dictionary
fPush ParamVar,  .end  ; -> {S: [DP] (before create), T: ParamVar (param type)}
fDo   Here,      .end  ; -> {[DP] (old), S: ParamVar, T: [DP] (now)}
fDo   ByteStore, .end  ; -> {T: [DP] (old value from before create)}
fPush 1,         .end  ; -> {S: [DP] (old), T: 1}        (allot for param type)
fDo   Allot,     .end  ; -> {T: [DP] (old)}
fPush 0,         .end  ; -> {S: [DP] (old), T: 0}
fDo   Here,      .end  ; -> {[DP] (old), S: 0, T: [DP] (now)}
fDo   Store,     .end  ; -> {T: [DP] (old)}               (initialize var to 0)
fPush 4,         .end  ; -> {S: [DP] (old), T: 4}        (allot for 32-bit var)
fDo   Allot,     .end  ; -> {T: [DP] (old)}
fPush Last,      .end  ; -> {S: [DP] (old), T: Last}
fDo   WordStore, .end  ; -> {}                      (update head of dictionary)
.end:
ret

mConstant:             ; CREATE -- create name, store T in dictionary, allot 4
fDo   Here,      .end  ; -> {S: number, T: [DP] (ptr to new dictionary entry)}
fDo   Swap,      .end  ; -> {S: [DP] (ptr to new dictionary entry), T: number}
fDo   Create,    .end  ; Read name from input stream and add it to dictionary
fPush ParamConst, .end ; -> {[DP] (old), S: number, T: ParamConst (type)}
fDo   Here,      .end  ; -> {[DP] (old), number, S: ParamConst, T: [DP] (now)}
fDo   ByteStore, .end  ; -> {S: [DP] (old), T: number}
fPush 1,         .end  ; -> {[DP] (old), S: number, T: 1}      (allot for type)
fDo   Allot,     .end  ; -> {S: [DP] (old), T: number}
fDo   Here,      .end  ; -> {[DP] (old), S: number, T: [DP] (now)}
fDo   Store,     .end  ; -> {T: [DP] (old)}                 (store const value)
fPush 4,         .end  ; -> {S: [DP] (old), T: 4}
fDo   Allot,     .end  ; -> {T: [DP] (old)}            (allot for 32-bit const)
fPush Last,      .end  ; -> {S: [DP] (old), T: Last}
fDo   WordStore, .end  ; -> {}                      (update head of dictionary)
.end:
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

mComma:                ; COMMA -- Store T at end of dictionary and allot 4
fDo   Here,      .end  ; -> {S: number, T: [DP] (address of first free byte)}
fDo   Store,     .end  ; -> {}
fPush 4,         .end  ; -> {T: 4}
fDo   Allot,     .end  ; -> {}
.end:
ret

mHere:                 ; HERE -- Push address of first free dictionary byte
fPush DP,        .end  ; -> {T: DP (address of DP)}
fDo   WordFetch, .end  ; -> {T: [DP] (address of first free dictionary byte)}
.end:
ret

mQuestion:             ; QUESTION -- fetch and pint (shorthand for `@ .`)
fDo   Fetch,     .end  ; {T: address} -> {T: [address]}
fDo   Dot,       .end  ; -> {}
.end:
ret


;-----------------------------
; Dictionary: Stack ops

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


;-----------------------------
; Return stack, call, jumps

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

mClearReturn:                 ; Clear the return stack
xor rdi, rdi
movq RSDeep, rdi
ret

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

mNext:                ; If R is zero, drop R and return, otherwise decrement R
movq rdi, RSDeep              ; make sure return stack is not empty
cmp dil, 1
jb mErr20ReturnUnderflow
test R, R                     ; check if R is 0
jz .doneRet                   ; if so: finish up
dec R                         ; else: decrement R
ret
.doneRet:                     ; end of loop: drop R and return
call mRPopW
test VMFlags, VMErr
jz mReturn
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


;-----------------------------
; Dictionary: Math ops

mMathDrop:                    ; Shared drop preamble for 2-operand math ops
movq rdi, DSDeep              ; make sure there are 2 items on the stack
cmp dil, 2
jb mErr1Underflow
mov esi, T                    ; save value of old top item
dec edi                       ; do a drop
movq DSDeep, rdi
dec edi
mov T, [DSBase+4*edi]
mov W, esi                    ; leave old T in eax (W) for use with math ops
ret

mPlus:                        ; +   ( 2nd T -- 2nd+T )
call mMathDrop
add T, W
ret

mMinus:                       ; -   ( 2nd T -- 2nd-T )
call mMathDrop
sub T, W
ret

mNegate:                      ; Negate T (two's complement)
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
neg T
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
call mMathDrop
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovl T, W                    ; if old 2nd was less, then use old T for new T
ret

mMin:                         ; MIN   ( 2nd T -- the_smaller_one )
call mMathDrop
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovg T, W                    ; if old 2nd was more, then use old T for new T
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

;-----------------------------
; Dictionary: Boolean ops
;
; == Important Note! ==
; Forth's boolean constants are not like C booleans. In Forth,
;   True value:   0 (all bits clear)
;   False value: -1 (all bits set)
; This allows for sneaky tricks such as using the `AND`, `OR`, `XOR`, and
; `INVERT` to act as both bitwise and boolean operators.
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

mInvert:                      ; Invert all bits of T (one's complement)
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
not T                         ; note amd64 not opcode is one's complement
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
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
mov W, T
xor T, T                      ; set T to false (-1)
dec T
xor edi, edi                  ; prepare value of true (0) in edi
test W, W                     ; check of old T<0 by setting sign flag (SF)
cmovs T, edi                  ; if so, change new T to true
ret

mZeroEqual:                   ; 0=   ( T -- bool_is_T_equal_0 )
movq rdi, DSDeep              ; need at least 1 item on stack
cmp dil, 1
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
mov word [Mem+Base], word 16
ret

mDecimal:                     ; Set number base to 10
mov word [Mem+Base], word 10
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
test VMFlags, VMCompile  ; check if compile mode is active
jz mPush                 ; if not compiling: push the number
;---------------------
.compileLiteral:         ; else: compile number into code memory
movzx ecx, word [Mem+CodeP]  ; check that: Heap < [CodeP] < HeapEnd-5
cmp ecx, Heap
jb mErr13AddressOOR
cmp ecx, HeapEnd-5
jnb mErr15HeapFull       ; stop if code memory is full
lea edi, [Mem]           ; else: prepare destination pointer
cmp W, 0                 ; find most compact literal to accurately store W
jl .compileNegative
cmp W, 255
jbe .compileU8           ; number fits in 0..255     --> use U8
cmp W, 65535
jbe .compileU16          ; fits in 256..65535        --> use U16
jmp .compileI32          ; fits in 65535..2147483647 --> use I32
.compileNegative:
cmp W, -127
jge .compileI8           ; fits in -127..-1     --> use I8
cmp W, -32768
jge .compileI16          ; fits in -32768..-128 --> use I16 (otherwise, I32)
.compileI32:             ; compile as 4-byte signed literal
mov [edi+ecx], byte tI32
inc ecx
mov [edi+ecx], W
add ecx, 4
mov word [Mem+CodeP], cx
ret
.compileU16:             ; compile as 2-byte unsigned literal
mov [edi+ecx], byte tU16
inc ecx
mov [edi+ecx], WW
add ecx, 2
mov word [Mem+CodeP], cx
ret
.compileU8:              ; compile as 1-byte unsigned literal
mov [edi+ecx], byte tU8
inc ecx
mov [edi+ecx], WB
add ecx, 1
mov word [Mem+CodeP], cx
ret
.compileI16:             ; compile as 2-byte signed literal
mov [edi+ecx], byte tI16
inc ecx
mov [edi+ecx], WW
add ecx, 2
mov word [Mem+CodeP], cx
ret
.compileI8:              ; compile as 1-byte signed literal
mov [edi+ecx], byte tI8
inc ecx
mov [edi+ecx], WB
add ecx, 1
mov word [Mem+CodeP], cx
ret
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


;-----------------------------
; Dictionary: Fetch and Store

mFetch:                     ; Fetch: pop addr, load & push dword [Mem+T]
movq rdi, DSDeep            ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
test T, T                   ; check address satisfies: 0 <= T < MemSize-3
jl mErr13AddressOOR
cmp T, MemSize-3
jge mErr13AddressOOR
mov T, [Mem+T]              ; pop addr, load dword, push dword
ret

mStore:                     ; Store dword (second) at address [Mem+T]
movq rsi, DSDeep            ; make sure stack depth >= 2 items (data, address)
cmp sil, 2
jb mErr1Underflow
cmp T, Fence               ; check address satisfies: Fence < T < MemSize-3
jle mErr13AddressOOR
cmp T, MemSize-3
jge mErr13AddressOOR
mov edi, T                  ; save address
dec rsi                     ; drop address
mov T, [DSBase+4*esi-4]     ; T now contains the data dword from former second
mov [Mem+edi], T            ; store data at [Mem+T]
dec rsi                     ; drop data dword
movq DSDeep, rsi
mov T, [DSBase+4*esi-4]
ret

mByteFetch:                 ; Fetch: pop addr, load & push byte [Mem+T]
movq rdi, DSDeep            ; make sure stack has at least 1 item (address)
cmp dil, 1
jb mErr1Underflow
test T, T                   ; check address satisfies: 0 <= T < MemSize
jl mErr13AddressOOR
cmp T, MemSize
jge mErr13AddressOOR
xor W, W                    ; pop addr, load dword
mov WB, byte [Mem+T]
mov T, W                    ; push dword
ret

mByteStore:                 ; Store low byte of (second) at address [Mem+T]
movq rdi, DSDeep            ; make sure stack depth >= 2 items (data, address)
cmp dil, 2
jb mErr1Underflow
cmp T, Fence                ; check address satisfies: Fence < T < MemSize
jle mErr13AddressOOR
cmp T, MemSize
jge mErr13AddressOOR
mov esi, T                  ; save address
dec edi                     ; drop address
mov T, [DSBase+4*edi-4]     ; T now contains the data from former second
mov byte [Mem+esi], TB      ; store data at [Mem+T]
dec edi                     ; drop data dword
movq DSDeep, rdi
mov T, [DSBase+4*edi-4]
ret

mWordFetch:                 ; Fetch: pop addr, load & push word [Mem+T]
movq rdi, DSDeep            ; make sure stack has at least 1 item (address)
cmp dil, 1
jb mErr1Underflow
test T, T                   ; check address satisfies: 0 <= T < MemSize-1
jl mErr13AddressOOR
cmp T, MemSize-1
jge mErr13AddressOOR
xor W, W                    ; pop addr, load dword
mov WW, word [Mem+T]
mov T, W                    ; push dword
ret

mWordStore:                 ; Store low word of (second) at address [Mem+T]
movq rdi, DSDeep            ; make sure stack depth >= 2 items (data, address)
cmp dil, 2
jb mErr1Underflow
cmp T, Fence               ; check address satisfies: Fence < T < MemSize-1
jle mErr13AddressOOR
cmp T, MemSize-1
jge mErr13AddressOOR
mov esi, T                  ; save address
dec edi                     ; drop address
mov T, [DSBase+4*edi-4]     ; T now contains the data from former second
mov word [Mem+esi], TW      ; store data at [Mem+T]
dec edi                     ; drop data dword
movq DSDeep, rdi
mov T, [DSBase+4*edi-4]
ret


;-----------------------------
; Dictionary: Strings

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


;-----------------------------
; Dictionary: Formatting

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
