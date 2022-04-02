; Attempt at a Forth-like REPL

bits 64
default rel
global _start

;=============================
section .bss
;=============================

rStackLo: resq 31 ; Return stack holds 64-bit qword addresses
rStackHi: resq 1
inBuf: resb 256   ; Keyboard input buffer; byte 0 is length
strBuf: resb 256  ; String scratch buffer; byte 0 is length

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

%define W r11    ; Working register, full qword
%define WD r11d  ; Working register, low dword
%define WB r11b  ; Working register, low byte

                ; Data stack is 4 registers, lol
%define T r12   ; Top on stack
%define S r13   ; Second on stack
%define X r14   ; Third on stack
%define Y r15   ; Forth on stack

%macro mPtr 1  ; PTR - Push a qword pointer literal
  lea W, %1
  call mPush
%endmacro

%macro mLitD 1  ; LITD - Push a dword literal with zero extend
  mov WD, %1
  call mPush
%endmacro

%macro mLitB 1  ; LITB - Push a byte literal with zero extend
  movzx WB, %1
  call mPush
%endmacro

%macro mNext 0  ; NEXT - Return from word
  ret
%endmacro

;-----------------------------
_start:

; Set up the return stack which grows downward
mov rsp, rStackHi

; Fake feeding a sequence of input words to the outer interpreter
mPtr d_version  ; Print version string normal
call putln
mPtr strBuf     ; Reverse version string into string buffer
mPtr d_version
call reverse
call putln      ; Print string buffer
mPtr strBuf     ; Reverse scratch buffer into itself
call mDup
call reverse
call putln      ; Print string buffer
mPtr d_prompt   ; Print prompt
call puts
call gets       ; Read an input line then print it
mPtr inBuf
call puts       ; (buffer includes newline)
mPtr d_prompt   ; Print prompt
call puts
call gets       ; Read an input line then print it
mPtr inBuf
call puts       ; (buffer includes newline)

jmp exit

;-----------------------------
dictionary:

mDup:     ; DUP - Push copy of T to data stack
mov W, T  ; implicit tail call to mPush

mPush:    ; PUSH - Push W to data stack
mov Y, X
mov X, S
mov S, T
mov T, W
ret

mDrop:    ; DROP - discard T
mov T, S
mov S, X
mov X, Y
ret

putln:               ; Write string [T] to stdout, then add a newline
call puts
mPtr d_newline       ; implicit tail call to puts

puts:                ; Write string [T] to stdout
mov rax, sys_write   ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
mov rdi, stdout      ; fd
mov rsi, T           ; *buf (string starts at second byte of string record)
inc rsi
movzx rdx, byte [T]  ; count (string length is first byte of string record)
syscall
jmp mDrop            ; tail call through mDrop

gets:                ; Read string to strBuf from stdin
mov rax, sys_read    ; rax=sys_read(rdi: fd, rsi: *buf, rdx: count)
mov rdi, stdin
mov rsi, inBuf       ; reserve first byte for length
inc rsi
mov dx, 255
syscall
mov byte [inBuf], al  ; set length as return value 
ret

reverse:             ; Reverse src string [T] into dest buffer [S]
movzx rcx, byte [T]  ; src length (255 max)
mov rsi, T           ; src_ptr, range (src+len(src))..(src+1)
add rsi, rcx
mov rdi, S           ; dest_ptr, range (dest+1)..(dest+len(src))
mov [rdi], cl        ; dest length := src length
inc rdi
cmp T, S             ; If (src == dest) then reverse in place
je reverse_in_place
reverse_src_dest:    ; Algorithm 1: separate src and dest
mov al, [rsi]        ; copy 1 byte from end of src to start of dest
mov [rdi], al
dec rsi              ; dec src_ptr
inc rdi              ; inc dest_ptr
cmp rsi, T
ja reverse_src_dest  ; loop while src_ptr > src
jmp reverse_end
reverse_in_place:
mov al, [rsi]        ; In place algorithm
mov WB, [rdi]
mov [rdi], al
mov [rsi], WB
dec rsi
inc rdi
cmp rsi, rdi         ; loop until pointers cross
ja reverse_in_place
reverse_end:
jmp mDrop            ; tail call through DROP, leaving dest buf ptr) as new T

exit:                ; Exit process
mov rax, sys_exit    ; rax=sys_exit(rdi: code)
mov rdi, 0           ; exit code
syscall

;=============================
section .data
;=============================

; First byte is length of string
d_newline: db 1, 10
d_version: db 18, "Markab REPL v0.0.1"
d_prompt: db 2, "> "
