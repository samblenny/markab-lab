; Print "Hello, world!\n" and exit with success.
;
; This plays games with sometimes using 8-bit registers (AL, DIL, DL) instead
; of the 64-bit registers (RAX, RDI, RSI, RDX) in order to save some bytes in
; the generated code. According to the AMD64 Architecture Programmer's Manual
; (p27 fig3-3), 8-bit operands do not modify high bits, so this starts by
; zeroing out the 64-bit registers. I think this is okay? Seems to work.

BITS 64

%define sys_write 1
%define sys_exit 60
%define stdout 1

global _start

section .text
_start:


xor eax, eax       ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
inc eax            ; rax=1 means sys_write
mov edi, eax       ; rdi=1 means stdout
lea esi, [dword str] ; *buf
xor edx, edx
mov dl, 14         ; count
syscall

mov al, sys_exit   ; rax=sys_exit(rdi: code)
xor edx, edx       ; exit code = 0
syscall

section .data
str:
db "hello, world!", 10
