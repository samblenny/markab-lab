; Minimal program: just invoke sys_exit with an exit code

bits 64
global _start

section .text
_start:
mov rdi, 3    ; set exit code
mov rax, 60   ; select sys_exit
syscall

