; Fancier token threaded inner interpreter with word labels adjusted to be a
; bit more like figForth (Pad for PAD, DotQuote for .", DotS for .S, ...).
;
; Output from `make token2.run`:
; ```
; Hello, world!

;  T 0123ABCD
;  S 00000043
; 03 00000043
; 04 00000042
; 05 00000041
; 06 FFFFFFFD
; 07 FFFFFFFE
; 08 FFFFFFFF
; 09 00000005
; 0A 00000004
; 0B 00000003
; 0C 00000002
; 0D 00000001
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
initROM:
db 1,1,  1,2,  1,3       ; 1  2  3            ( bytes)
db 2,4,0,  2,5,0         ; 4  5               ( words)
db 3,-1,-1,-1,-1         ; -1                 ( dword)
db 3,-2,-1,-1,-1         ; -2                 ( dword)
db 3,-3,-1,-1,-1         ; -3                 ( dword)
db 1,'A',  1,'B',  1,'C' ; 'A'  'B'  'C'      ( bytes)
db 4                     ; Dup
db 3                     ; 0x0123abcd         ( dword)
dd 0x0123abcd
db 0                     ; Nop
db 9                     ; ."
dw 13                    ;            ( string length)
db "Hello, world!"       ; Hello, world"
db 1, 10                 ; 10                  ( byte)
db 10                    ; Emit           ( manual CR)
db 8                     ; .S   ( non-dest stack dump)
db 5, 5, 5, 5, 5, 5, 5,  ; Drop Drop ...  ( many drop)
db 5, 5, 5, 5, 5, 5, 5   ;    ( fully empty the stack)
db 8                     ; .S ( nop since stack empty)
db 13                    ; Exit
.end:


;-----------------------------
; Jump table

%macro jt 2
jt%[%1]: dw m%[%2] - DictBase
%endmacro

align 16, db 0
db "== JumpTable: =="
align 16, db 0

JumpTable:
jt  0, Nop
jt  1, LitB
jt  2, LitW
jt  3, LitD
jt  4, Dup
jt  5, Drop
jt  6, Swap
jt  7, Over
jt  8, DotS
jt  9, DotQuote
jt 10, Emit
jt 11, CR
jt 12, Dot
jt 13, Exit

jtMax: db 13
align 16, db 0
db "=== EndJumpT ==="
align 16, db 0

;-----------------------------
; Strings {dword len, chars}

datBadToken1: db 11, 0, "BadToken:  "
datBadToken2: db  4, 0, "  I:"


;=============================
section .bss
;=============================

; Data stack
align 16, resb 0
%define DStackSize 16
DStackLo: resq DStackSize  ; Data stack (cell size is 64-bit qword)
DStackHi: resq 1

; String buffers
align 16, resb 0
%define strMax 1021
TIB: resb 2+strMax+1     ; Terminal input buffer; word 0 is length
Pad: resb 2+strMax+1     ; String scratch buffer; word 0 is length


;=============================
section .text
;=============================


;-----------------------------
; Stack macros

%define W r10d       ; Working register, 32-bit zero-extended dword
%define WB r10b      ; Working register, low byte
%define X r11        ; Temporary register (not preserved during CALL)

%define T r12d       ; Top on stack, 32-bit zero-extended dword
%define TB r12b      ; Top on stack, low byte
%define S r13d       ; Second on stack, 32-bit zero-extended dword
%define SB r13b      ; Second on stack, low byte
%define DSHead r14d  ; Index to head of circular data stack
%define DSLen r15d   ; Current depth of circular data stack (excludes T & S)


;-----------------------------
_start:


;-----------------------------
; Init registers & buffers

initStack:                    ; Init data stack registers
xor eax, eax
xor T, eax
mov S, eax
mov DSHead, eax
mov DSLen, eax

initPad:                      ; Init string buffers
mov [TIB], eax
mov [Pad], eax


;//////////////////////////////
doInner:
;//////////////////////////////
mov ebp, initROM              ; persist instruction pointer (I) in rbp
mov ebx, JumpTable            ; persist jump table address in rbx
align 16                      ; align loop to a fresh cache line
.while:
movzx ecx, byte [rbp]         ; load token at I
cmp cl, byte [jtMax]          ; break to debug if token is not in range
ja .eBadToken
movzx esi, word [rbx+2*rcx]   ; fetch jump table address offset
add esi, DictBase             ; calculate jump address
inc ebp                       ; advance I
call rsi                      ; jump (callee may adjust I for LITx)
jmp .while                    ; LOOP FOREVER! ({bad|exit} token can break)

;//////////////////////////////
.eBadToken:                   ; Exit with debug message about bad token
call mDotS                    ; dump stack
lea W, [datBadToken1]         ; print bad token error label
call mStrPut.W
movzx W, byte [rbp]           ; print offending token value
call mDotB.W
lea W, [datBadToken2]         ; print token's instruction pointer
call mStrPut.W
mov W, ebp
sub W, initROM
call mDotB.W
call mCR
jmp mExit                     ; exit
;/////////////////////////////


;-----------------------------
; Dictionary base address
DictBase:
nop

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
movzx eax, word [rbp]  ; get length of string in bytes (for adjusting I)
add ax, 2              ;   add 2 for length dword
mov W, ebp             ; I (ebp) should be pointing to {length, chars}
add ebp, eax           ; adjust I past string
jmp mStrPut.W


;-----------------------------
; Dictionary: Stack ops

mNop:                  ; NOP - do nothing
ret

mDup:                  ; DUP - Push T
mov W, T
jmp mPush

mSwap:                 ; SWAP - Swap T and S
xchg T, S
ret

mOver:                 ; OVER - Push S
mov W, S
jmp mPush

mPush:                 ; PUSH - Push W to data stack
mov edx, DStackLo
mov dword [edx+4*DSHead], S  ; on entry, DSHead points to an availble cell
inc DSHead
and DSHead, 0x0f       ; modulo 16 because data stack is circular
inc DSLen              ; limit length to max capacity of data stack (16)
mov eax, 18
cmp DSLen, eax         ; bascially, DSLen==18 probably indicates an error
cmova DSLen, eax
mov S, T
mov T, W
ret

mDrop:                 ; DROP - discard (pop) T
cmp DSLen, 1           ; return silently if stack is empty
jb .done
mov T, S
mov edx, DStackLo
mov edi, 15
add DSHead, edi        ; equivalent to (DSHead + 16 - 1) % 16
and DSHead, edi
mov S, dword [edx+4*DSHead]
dec DSLen              ; make sure DSLen doesn't go lower than 0
.done:
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

mDotS:                ; Nondestructively print hexdump of stack
push rbx              ; use rbx & rbp to preserve values across calls
push rbp
cmp DSLen, 0                   ; return if stack is empty
je .done
lea edi, [Pad]                 ; format T if depth >= 1
mov word [rdi], 4              ;   print label for T
mov dword [rdi+2], 0x2054200A  ;   CR, " T "
mov W, dword Pad
call mStrPut.W
mov W, T                       ;   print value of T
call mDot.W
cmp DSLen, 1                   ; format S if depth >= 2
je .done
lea edi, [Pad]
mov word [rdi], 4              ;   print label for S
mov dword [rdi+2], 0x2053200A  ;   CR, " S "
mov W, dword Pad
call mStrPut.W
mov W, S                       ;   print value of S
call mDot.W
call mCR
cmp DSLen, 2
je .done
lea ebx, [DSLen-2]    ; format the rest if depth > 2
mov ebp, DSHead
.for:
mov W, DSLen          ; print the stack depth label
sub W, ebx
inc W
call mDotB.W
call mSpace
mov edi, 15           ; step 1 cell down the circular data stack
add ebp, edi          ; equivalent to (DSHead + 16 - 1) % 16
and ebp, edi
mov esi, DStackLo         ; set this each time because of calls below
mov W, dword [rsi+4*rbp]  ; peek at the current cell's value
call mDot.W               ; print it
call mCR
dec ebx
jnz .for
.done:
pop rbp
pop rbx
ret


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
xor eax, eax           ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
inc eax                ; rax=1 means sys_write
mov edi, eax           ; rdi=1 means fd 1, which is stdout
mov esi, W             ; *buf
movzx edx, word [rsi]  ; count (string length is first word of string record)
add esi, 2             ; string data area starts at third byte of Pad
alignSyscall
ret

mExit:                 ; Exit process
mov eax, sys_exit      ; rax=sys_exit(rdi: code)
xor edi, edi           ; exit code = 0
and esp, -16           ; align stack to System V ABI (maybe unnecessary?)
syscall
