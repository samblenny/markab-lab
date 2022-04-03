; Test with return stack using SP and data stack using R14 & R15.
;
; The data stack is circular so segfaults from overflow or underflow can't
; happen. But, overflow and underflow will still produce logic errors. This
; does not attempt to prevent overflow or underflow on the return stack, so
; that one can segfault. This code uses balanced CALL and RET instructions.
; Previously I experimented with jumpless implicit tail calls (continuations)
; and JMP tail calls, but that was making me really nervous. It would be too
; easy to make editing errors that foul up the control flow.
;
; Sample output:
; ```
; 9*********
; 8********
; 7*******
; 6******
; 5*****
; 4****
; 3***
; 2**
; 1*
; 1*
; 1*
; 1*
; 1*
; 2**
; 5*****
; 4****
; 6******
; 7*******
; ```

bits 64
default rel
global _start

;=============================
section .bss
;=============================

%define RStackSize 63
%define DStackSize 16

RStackLo: resq RStackSize  ; Return stack holds 64-bit qword addresses
RStackHi: resq 1
DStackLo: resq DStackSize  ; Data stack (cell size is 64-bit qword)
DStackHi: resq 1
align 8, db 0
inBuf: resb 256  ; Keyboard input buffer; byte 0 is length
strBuf: resb 256 ; String scratch buffer; byte 0 is length

;=============================
section .text
;=============================

;-----------------------------
; Syscall constants

%define sys_read 0
%define sys_write 1
%define sys_exit 60
%define stdin 0
%define stdout 1

;-----------------------------
; Stack macros

%define W r11        ; Working register, full qword
%define WD r11d      ; Working register, low dword
%define WB r11b      ; Working register, low byte

%define T r12        ; Top on stack
%define S r13        ; Second on stack
%define DSHead r14   ; Index to head of circular data stack
%define DSLen r15    ; Current depth of circular data stack (excludes T & S)
%define mCELL 8      ; Data stack cell size

%macro mPtr 1        ; PTR - Push a qword pointer literal
  lea W, %1
  call mPush
%endmacro

%macro mLitD 1       ; LITD - Push a dword literal with zero extend
  mov WD, %1
  call mPush
%endmacro

%macro mLitB 1       ; LITB - Push a byte literal with zero extend
  movzx WB, %1
  call mPush
%endmacro

;-----------------------------
_start:

; Set up the return stack which grows downward
mov edx, RStackLo
mov edi, RStackSize  ; This assumes 1 extra qword at RStackHi
mov rsi, exit
reset_rstack:
mov [edx+edi], rsi
dec edi
jnz reset_rstack
mov rsp, RStackHi

; Set up data stack which grows upward
mov edx, DStackLo
mov edi, DStackSize   ; This assumes 1 extra qword at DStackHi
xor rsi, rsi
reset_dstack:
mov [edx+edi], rsi
dec edi
jnz reset_dstack
mov DSHead, 0
mov DSLen, 0

; Do the stuff
mPtr dat9     ; This gets lost due to too many pushes
mPtr dat8     ; This gets lost due to too many pushes
mPtr dat7
mPtr dat6
mPtr dat5
mPtr dat4
mPtr dat3
call mDrop    ; zap the 3
call mSwap    ; swap 4 & 5
mPtr dat2
mov rcx, 5
ones_loop:
mPtr dat1
dec rcx
jnz ones_loop
mPtr dat2
mPtr dat3
mPtr dat4
mPtr dat5
mPtr dat6
mPtr dat7
mPtr dat8
mPtr dat9

mov rcx, DSLen         ; loop stackDepth times
begin_print_stack:     ; Print stack, assuming it's full of string pointers
push rcx
call putln
pop rcx
dec rcx
jnz begin_print_stack

jmp exit

;-----------------------------
dictionary:

mDup:                  ; DUP - Push copy of T to data stack
mov W, T
call mPush
ret

mPush:                 ; PUSH - Push W to data stack
mov rdx, DStackLo
mov [rdx+8*DSHead], S  ; on entry, DSHead points to an availble cell
inc DSHead
and DSHead, 0x0f       ; Modulo 16 because data stack is circular
inc DSLen              ; Limit length to max capacity of data stack (16)
mov rax, 18
cmp DSLen, rax         ; Bascially, DSLen==18 probably indicates an error
cmova DSLen, rax
mov S, T
mov T, W
ret

mDrop:                 ; DROP - discard T
cmp DSLen, 0
mov T, S
mov rdx, DStackLo
mov rdi, 15
add DSHead, rdi        ; Equivalent to (DSHead + 16 - 1) % 16
and DSHead, rdi
mov S, [rdx+8*DSHead]
mov rax, DSLen         ; Make sure DSLen doesn't go lower than 0
dec rax
cmovns DSLen, rax
ret

mSwap:                 ; SWAP - exchange T and S
xchg T, S
ret

putln:                 ; Write string [T] to stdout, then add a newline
call puts
mPtr datCR
call puts
ret

puts:                  ; Write string [T] to stdout
;-----------------
; mov rdx, strBuf      ; Uncomment this to print pointers (for |hexdump -C)
; mov byte [rdx], 8    ; instead of printing from dereferenced pointers
; mov [rdx+1], T
; mov T, rdx
;-----------------
mov rax, sys_write     ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
mov rdi, stdout        ; fd
mov rsi, T             ; *buf (string starts at second byte of string record)
inc rsi
movzx rdx, byte [T]    ; count (string length is first byte of string record)
syscall
call mDrop
ret

exit:                  ; Exit process
mov rax, sys_exit      ; rax=sys_exit(rdi: code)
mov rdi, 0             ; exit code
syscall

;=============================
section .data
;=============================

; First byte of strings is length (max 255)
datCR: db 1, 10
dat1: db  2, "1*"
dat2: db  3, "2**"
dat3: db  4, "3***"
dat4: db  5, "4****"
dat5: db  6, "5*****"
dat6: db  7, "6******"
dat7: db  8, "7*******"
dat8: db  9, "8********"
dat9: db 10, "9*********"
