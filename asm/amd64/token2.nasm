; Fancier token threaded inner interpreter with word labels adjusted to be a
; bit more like figForth (Pad for PAD, DotQuote for .", DotS for .S, ...).
;
; Output from `make token2.run`:
; ```
; Hello, world!
;
;  T 0123ABCD
; 02 00000042
; 03 00000042
; 04 00000041
; 05 FFFFFFFE
; 06 FFFFFFFF
; 07 00000003
; 08 00000002
; 09 00000001
; Error #1 Stack too empty
; Error #1 Stack too empty
; Stack is empty
; ```

bits 64
default rel
global _start

;=============================
section .data
;=============================

;-----------------------------
; Compiled code ROM (tokens)

align 16, db 0
db "=== initROM: ==="

align 16, db 0
LoadScreen:
db 2,2,  2,1             ; 2  1               ( bytes)
db 7                     ; Swap
db 3,3,0,                ; 3                   ( word)
db 4,-1,-1,-1,-1         ; -1                 ( dword)
db 4,-2,-1,-1,-1         ; -2                 ( dword)
db 2,'A',  2,'B'         ; 'A'  'B'           ( bytes)
db 5                     ; Dup
db 4                     ; 0x0123abcd         ( dword)
dd 0x0123abcd
db 0                     ; Nop
db 10                    ; ."
dw 13                    ;            ( string length)
db "Hello, world!"       ; Hello, world"
db 2, 10                 ; 10                  ( byte)
db 11                    ; Emit           ( manual CR)
db 9                     ; .S   ( non-dest stack dump)
db 6, 6, 6, 6, 6, 6, 6   ; Drop Drop ...  ( many drop)
db 6, 6, 6, 6            ;    ( fully empty the stack)
db 9                     ; .S           ( stack empty)
db 1                     ; Next
.end:


;-----------------------------
; Jump table

%macro jt 2
jt%[%1]: dd m%[%2]
%endmacro

align 16, db 0
db "== JumpTable: =="
align 16, db 0

JumpTable:
jt  0, Nop
jt  1, Next                   ; This gets handled specially by doInner
jt  2, LitB
jt  3, LitW
jt  4, LitD
jt  5, Dup
jt  6, Drop
jt  7, Swap
jt  8, Over
jt  9, DotS
jt 10, DotQuote
jt 11, Emit
jt 12, CR
jt 13, Dot
jt 14, Err3BadToken
jt 15, Err3BadToken
jt 16, Err3BadToken
jt 17, Err3BadToken
jt 18, Err3BadToken
jt 19, Err3BadToken
jt 20, Err3BadToken
jt 21, Err3BadToken
jt 22, Err3BadToken
jt 23, Err3BadToken
jt 24, Err3BadToken
jt 25, Err3BadToken
jt 26, Err3BadToken
jt 27, Err3BadToken
jt 28, Err3BadToken
jt 29, Err3BadToken
jt 30, Err3BadToken
jt 31, Err3BadToken

align 16, db 0
db "== EndJumpTbl =="
align 16, db 0


;-----------------------------
; Strings {dword len, chars}

datErr1se:   db 25, 0, "Error #1 Stack too empty", 10
datErr2sf:   db 24, 0, "Error #2 Stack too full", 10
datErr3btA:  db 22, 0, "Error #3 Bad token  T:"
datErr3btB:  db  4, 0, "  I:"
datErr4lt:   db 22, 0, "Error #4 Loop timeout", 10
datDotST:    db  4, 0, 10, " T "
datDotSNone: db 15, 0, "Stack is empty", 10


;=============================
section .bss
;=============================

align 16, resb 0              ; Data stack
%define DSMax 17              ; total size of data stack (T + 16 dwords)
DSBase: resd DSMax-1          ; data stack (excludes T; 32-bit dword cells)

align 16, resb 0              ; Return stack (for token interpreter)
%define RSMax 16              ; total size of return stack (16 dwords)
RSBase: resd RSMax            ; data stack (32-bit dword cells)

align 16, resb 0              ; String buffers
%define StrMax 1022           ; length of string data area
TIB: resb 2+StrMax            ; terminal input buffer; word 0 is length
align 16, resb 0
Pad: resb 2+StrMax            ; string scratch buffer; word 0 is length

align 16, resb 0              ; Error message buffers
ErrToken: resd 1              ; value of current token
ErrInst: resd 1               ; instruction pointer to current token


;=============================
section .text
;=============================


;-----------------------------
; Stack macros

%define W eax                 ; Working register, 32-bit zero-extended dword
%define WQ rax                ; Working register, 64-bit qword (for pointers)
%define WB al                 ; Working register, low byte

%define T r13d                ; Top on stack, 32-bit zero-extended dword
%define TB r13b               ; Top on stack, low byte
%define DSDeep r14d           ; Current depth of data stack (including T)
%define DSDeepB r14d          ; Current depth of data stack, low byte
%define RSDeep r15d           ; Current depth of return stack
%define RSDeepB r15d          ; Current depth of return stack, low byte


;-----------------------------
; Process entry point

_start:
xor W, W                      ; init data stack registers
mov T, W
mov DSDeep, W
mov RSDeep, W
mov [TIB], W                  ; init string buffers
mov [Pad], W
.loadScreen:                  ; run the load screen
mov W, LoadScreen
call doInner
.done:
jmp mExit


;-----------------------------
; Interpreters

doInner:                      ; Inner interpreter
push rbp
push rbx
mov ebp, W                    ; ebp = instruction pointer (I)
xor ebx, ebx                  ; max loop iterations = 2^32 - 1
dec ebx
align 16                      ; align loop to a cache line
.for:
;//////////////////////////////
movzx W, byte [rbp]           ; load token at I
cmp WB, 1                     ; handle NEXT (token = 1) specially
je .done
cmp WB, 32                    ; detect token beyond jump table range (CAUTION!)
jae mErr3BadToken
lea edi, [JumpTable]          ; fetch jump table address
mov esi, dword [rdi+4*WQ]
inc ebp                       ; advance I
call rsi                      ; jump (callee may adjust I for LITx)
dec ebx
jnz .for                      ; loop until timeout (or break by NEXT token)
;//////////////////////////////
.doneTimeout:                 ; alternate exit path when loop timed out
call mErr4LoopTimeout
.done:                        ; normal exit path
pop rbx
pop rbp
ret

;-----------------------------
; Dictionary: Error handling

mErr1Underflow:               ; Handle stack too empty error
lea W, [datErr1se]            ; print error message
call mStrPut.W
xor DSDeep, DSDeep            ; clear stack
ret                           ; return control to interpreter

mErr2Overflow:                ; Handle stack too full error
lea W, [datErr2sf]            ; print error message
call mStrPut.W
xor DSDeep, DSDeep            ; clear stack
ret                           ; return control to interpreter

mErr3BadToken:                ; Handle bad token error
movzx W, byte [rbp]           ; save value of token
mov [ErrToken], W
mov W, ebp                    ; save instruction pointer to token
sub W, LoadScreen
mov [ErrInst], W
lea W, [datErr3btA]           ; print error message
call mStrPut.W
mov W, [ErrToken]             ; print token value
call mDotB.W
lea W, [datErr3btB]           ; print token instruction pointer
call mStrPut.W
mov W, [ErrInst]
lea ecx, [LoadScreen]
sub W, ecx
call mDotB.W
call mCR
jmp mExit                     ; exit

mErr4LoopTimeout:             ; Handle loop timeout error
lea W, [datErr4lt]            ; print error message
call mStrPut.W
ret                           ; return control to interpreter


;-----------------------------
; Dictionary: Literals

mLitB:                 ; LITB - Push an 8-bit byte literal with zero extend
movzx W, byte [rbp]    ; read literal from token stream
inc ebp                ; ajust I
jmp mPush

mLitW:                 ; LITW - Push a 16-bit word literal with zero extend
movzx W, word [rbp]    ; read literal from token stream
add ebp, 2             ; adjust I
jmp mPush

mLitD:                 ; LITD - Push a 32-bit dword literal with zero extend
mov W, dword [rbp]     ; read literal from token stream
add ebp, 4             ; adjust I
jmp mPush

mDotQuote:             ; Print string literal to stdout
movzx ecx, word [rbp]  ; get length of string in bytes (for adjusting I)
add cx, 2              ;   add 2 for length dword
mov W, ebp             ; I (ebp) should be pointing to {length, chars}
add ebp, ecx           ; adjust I past string
jmp mStrPut.W


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

mDotB:                ; Print T low byte to stdout (2 hex digits)
mov W, T
call mDrop
.W:                   ; Print W low byte to stdout (2 hex digits)
shl W, 24             ; shift low byte to high byte since mDot starts there
mov ecx, 2            ; set digit count
jmp mDot.W_ecx        ; use the digit conversion loop from mDot

mDot:                 ; Print T to stdout (8 hex digits)
mov W, T
call mDrop
.W:                   ; Print W to stdout (8 hex digits)
mov ecx, 8
.W_ecx:               ; Print some (all?) of W to stdout (ecx hex digits)
lea edi, [Pad]        ; set string struct length in Pad
mov word [rdi], cx
add edi, 2            ; advance dest ptr to start of string bytes
.for:
mov r8d, W            ; get the high nibble of W
shr r8d, 28
shl W, 4              ; shift that nibble off the high end of W
add r8d, '0'          ; convert nibble assuming its value is in 0..9
mov r9d, r8d
add r9d, 'A'-'0'-10   ; but, if value was >= 10, use hex digit instead
cmp r8b, '9'
cmova r8d, r9d
mov byte [rdi], r8b   ; append digit to string bytes of [Pad]
inc edi
dec ecx
jnz .for
lea W, [Pad]          ; print it
jmp mStrPut.W

mDotS:                        ; Nondestructively print hexdump of stack
push rbp
mov ecx, DSDeep
cmp cl, 0
je .empty
cmp cl, 1                     ; format T if data stack depth >= 1
jb .done
lea W, [datDotST]             ; T gets special label since it's not a number
call mStrPut.W
mov W, T                      ; prepare for printing T's value
xor ebp, ebp                  ; start loop counter at 1, for T's iteration
inc ebp
jmp .forPrintValue            ; for T, skip past numeric label & memory fetch
.for:
mov W, ebp
call mDotB.W                  ; print stack depth numeric label (2 is below T)
call mSpace
mov W, DSDeep                 ; fetch stack value (this gets skipped for T)
sub W, ebp
mov W, [DSBase+4*W]
.forPrintValue:               ; print the value (for both T and memory items)
call mDot.W
call mCR
inc ebp
cmp ebp, DSDeep
jbe .for                      ; loop in range 1..DSDeep
.done:                        ; clean up (normal exit point)
pop rbp
ret
.empty:                       ; alternate exit point for case of empty stack
pop rbp
lea W, [datDotSNone]
jmp mStrPut.W


;-----------------------------
; Dictionary: Syscalls

%define sys_read 0
%define sys_write 1
%define sys_exit 60
%define stdin 0
%define stdout 1

%macro alignSyscall 0  ; Syscall with manual 16-byte align of esp
  push rbp             ; preserve old rbp and rsp
  mov rbp, rsp
  and esp, -16         ; align rsp
  syscall              ; align stack to System V ABI (maybe unnecessary?)
  mov rsp, rbp         ; restore previous rsp and rbp
  pop rbp
%endmacro

mStrPut:               ; Write string [Pad] to stdout, clear [Pad]
lea W, [Pad]
.W:                    ; Write string [W] to stdout, clear [W]
mov esi, W             ; *buf (note: W is eax, so save it first)
xor eax, eax           ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
inc eax                ; rax=1 means sys_write
mov edi, eax           ; rdi=1 means fd 1, which is stdout
movzx edx, word [rsi]  ; count (string length is first word of string record)
add esi, 2             ; string data area starts at third byte of Pad
alignSyscall
ret

mExit:                 ; Exit process
mov eax, sys_exit      ; rax=sys_exit(rdi: code)
xor edi, edi           ; exit code = 0
and esp, -16           ; align stack to System V ABI (maybe unnecessary?)
syscall
