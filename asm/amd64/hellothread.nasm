; Baby steps toward building a threaded interpreter

BITS 64

%define sys_write 1
%define sys_exit 60
%define stdout 1

global _start

section .text
_start:

inst1: mov r8, d_hello  ; r8: top of stack
       mov r15, inst2   ; r15: next instruction
       jmp puts
inst2: mov r15, inst3
       jmp newline
inst3: mov r8, buffer   ; r8: top of stack
       mov r9, d_hello  ; r9: second on stack
       mov r15, inst4   ; r15: next instruction
       jmp reverse
inst4: mov r8, buffer
       mov r15, inst5
       jmp puts
inst5: mov r15, exit
       jmp newline

jmp exit

; =========================
; == Start of dictionary ==
; =========================

newline:            ; Write a newline to stdout
mov r8, d_newline   ; fall through to puts

puts:               ; Write string [r8] to stdout
mov rax, sys_write  ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
mov rdi, stdout     ; fd
mov rsi, r8         ; *buf (string starts at second byte of string record)
inc rsi
xor rdx, rdx        ; prepare for fetching 1-byte length
mov dl, [r8]        ; count (string length is first byte of string record)
syscall
jmp r15             ; NEXT

reverse:            ; Reverse src string [r9] into dest buffer [r8]
xor r10, r10        ; r10 := src length (1 byte max)
mov r10b, [r9]
mov r11, r9         ; r11 := src_ptr, range (src+len(src))..(src+1)
add r11, r10
mov r12, r8         ; r12 := dest_ptr, range (dest+1)..(dest+len(src))
mov [r12], r10b     ; dest length := src length
inc r12
reverse_loop:
mov r13, [r11]      ; copy 1 byte from end of src to start of dest
mov [r12], r13
dec r11             ; dec src_ptr
inc r12             ; inc dest_ptr
cmp r11, r9
ja reverse_loop     ; loop while src_ptr > src
jmp r15             ; NEXT

exit:               ; Exit process
mov rax, sys_exit   ; rax=sys_exit(rdi: code)
mov rdi, 0          ; exit code
syscall

; ==========
; == .BSS ==
; ==========

section .bss
buffer: resb 256    ; 255 byte scratch string buffer, first byte is length

; ===========
; == .DATA ==
; ===========

section .data
; First byte is length of string
d_hello: db 13, "hello, world!"
d_newline: db 1, 10
