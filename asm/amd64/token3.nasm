; Add math operations to threaded inner interpreter.
;
; Output from `make token3.run`:
; ```
; 2 1 + .S
;  T 00000003
; Drop 1 3 - .S
;  T FFFFFFFE
; Drop 3 11 * .S
;  T 00000021
; Drop 11 3 / .S
;  T 00000003
; Drop 11 3 Mod .S
;  T 00000002
; Drop 11 3 /Mod .S
;  T 00000003
; 02 00000002
; Drop Drop -11 3 /Mod .S
;  T FFFFFFFD
; 02 FFFFFFFE
; Max 2 5 Max .S
;  T 00000005
; 02 FFFFFFFE
; Min 3 1 Min .S
;  T 00000001
; 02 FFFFFFFE
; Abs Swap Abs Swap .S
;  T 00000001
; 02 00000002
; Drop Drop 15 5 And 15 5 Or 15 5 Xor .S
;  T 0000000A
; 02 0000000F
; 03 00000005
; Drop Drop Drop 1 2 < 2 2 < 1 -1 < .S
;  T FFFFFFFF
; 02 FFFFFFFF
; 03 00000000
; Drop Drop Drop 1 2 > 2 2 > 1 -1 > .S
;  T 00000000
; 02 FFFFFFFF
; 03 FFFFFFFF
; Drop Drop Drop -1 0< 0 0< 5 0< .S
;  T FFFFFFFF
; 02 FFFFFFFF
; 03 00000000
; Drop Drop Drop -1 0= 0 0= 5 0= .S
;  T FFFFFFFF
; 02 00000000
; 03 FFFFFFFF
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

%macro dbDotQuote 1           ; Compile a `." ..."` string with correct length
  %strlen %%mStrLen %1
  db 10
  dw %%mStrLen
  db %1
%endmacro

align 16, db 0
LoadScreen:                   ; Hand compiled load screen code
dbDotQuote "2 1 + .S"
db 2,2, 2,1, 14, 9
dbDotQuote "Drop 1 3 - .S"
db 6, 2,1, 2,3, 15, 9
dbDotQuote "Drop 3 11 * .S"
db 6, 2,3, 2,11, 16, 9
dbDotQuote "Drop 11 3 / .S"
db 6, 2,11, 2,3, 17, 9
dbDotQuote "Drop 11 3 Mod .S"
db 6, 2,11, 2,3, 18, 9
dbDotQuote "Drop 11 3 /Mod .S"
db 6, 2,11, 2,3, 19, 9
dbDotQuote "Drop Drop -11 3 /Mod .S"
db 6, 6, 4,-11,-1,-1,-1, 2,3, 19, 9
dbDotQuote "Max 2 5 Max .S"
db 20, 2,2, 2,5, 20, 9
dbDotQuote "Min 3 1 Min .S"
db 21, 2,3, 2,1, 21, 9
dbDotQuote "Abs Swap Abs Swap .S"
db 22, 7, 22, 7, 9
dbDotQuote "Drop Drop 15 5 And 15 5 Or 15 5 Xor .S"
db 6, 6, 2,15, 2,5, 23, 2,15, 2,5, 24, 2,15, 2,5, 25, 9
dbDotQuote "Drop Drop Drop 1 2 < 2 2 < 1 -1 < .S"
db 6, 6, 6, 2,1, 2,2, 27, 2,2, 2,2, 27, 2,1, 4,-1,-1,-1,-1, 27, 9
dbDotQuote "Drop Drop Drop 1 2 > 2 2 > 1 -1 > .S"
db 6, 6, 6, 2,1, 2,2, 28, 2,2, 2,2, 28, 2,1, 4,-1,-1,-1,-1, 28, 9
dbDotQuote "Drop Drop Drop -1 0< 0 0< 5 0< .S"
db 6, 6, 6, 4,-1,-1,-1,-1, 30, 2,0, 30, 2,5, 30, 9
dbDotQuote "Drop Drop Drop -1 0= 0 0= 5 0= .S"
db 6, 6, 6, 4,-1,-1,-1,-1, 31, 2,0, 31, 2,5, 31, 9

db 1                               ; Next
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
jt 14, Plus
jt 15, Minus
jt 16, Mul
jt 17, Div
jt 18, Mod
jt 19, DivMod
jt 20, Max
jt 21, Min
jt 22, Abs
jt 23, And
jt 24, Or
jt 25, Xor
jt 26, Not
jt 27, LessThan
jt 28, GreaterThan
jt 29, Equal
jt 30, ZeroLessThan
jt 31, ZeroEqual

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
;//////////////////////////////
.for:
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
cdq                           ; sign extend eax (W, old T) into rax
mov rdi, WQ                   ; save old T in rdi (use qword to prep for idiv)
mov W, T                      ; prepare dividend (old 2nd) in rax
cdq                           ; sign extend old 2nd into rax
idiv rdi                      ; signed divide 2nd/T (rax:quot, rdx:rem)
mov T, W                      ; new T is quotient from eax
ret

mMod:                         ; MOD   ( 2nd T -- <remainder 2nd/T> )
call mMathDrop                ; after drop, old value of T is in W
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

mLessThan:                    ; <   ( 2nd T -- bool_is_2nd_less_than_T )
call mMathDrop                ; drop leaves old T in W
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to false (-1), assuming 2nd >= T
dec T
xor esi, esi                  ; prepare true (0) in esi
cmp edi, W                    ; test for 2nd < T
cmovl T, esi                  ; if so, change new T to true
ret

mGreaterThan:                 ; >   ( 2nd T -- bool_is_2nd_greater_than_T )
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

mZeroLessThan:                ; 0<   ( T -- bool_is_T_less_than_0 )
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
