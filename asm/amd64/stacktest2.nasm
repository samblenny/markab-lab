; Test with return stack from exec + check argc, argv, and envp
;
; This does:
; 1. Dump initial stack frame from argc to the null at the end of the envp
;    array. Dump sequence is 8 bytes for pointer, then 8 bytes for its value.
; 2. Print "XXXXXXXXXXXXXXXX" marker
; 3. Dump saved values for frame pointer, argc, argv pointer, and envp pointer.
; 4. Print "XXXXXXXXXXXXXXXX" marker
;
; Since I don't have a hexdump word yet, I'm just printing raw bytes and piping
; them through `hexdump -C` with my shell.
;
; Sample output:
; ```
; $ make stacktest && ./stacktest | hexdump -C
; 00000000  80 ff c4 0c fe 7f 00 00  01 00 00 00 00 00 00 00  |................|
; 00000010  88 ff c4 0c fe 7f 00 00  f3 07 c5 0c fe 7f 00 00  |................|
; 00000020  90 ff c4 0c fe 7f 00 00  00 00 00 00 00 00 00 00  |................|
; 00000030  98 ff c4 0c fe 7f 00 00  ff 07 c5 0c fe 7f 00 00  |................|
; 00000040  a0 ff c4 0c fe 7f 00 00  0f 08 c5 0c fe 7f 00 00  |................|
; 00000050  a8 ff c4 0c fe 7f 00 00  30 08 c5 0c fe 7f 00 00  |........0.......|
; 00000060  b0 ff c4 0c fe 7f 00 00  3a 08 c5 0c fe 7f 00 00  |........:.......|
; 00000070  b8 ff c4 0c fe 7f 00 00  4f 08 c5 0c fe 7f 00 00  |........O.......|
; 00000080  c0 ff c4 0c fe 7f 00 00  5e 08 c5 0c fe 7f 00 00  |........^.......|
; 00000090  c8 ff c4 0c fe 7f 00 00  6b 08 c5 0c fe 7f 00 00  |........k.......|
; 000000a0  d0 ff c4 0c fe 7f 00 00  7c 08 c5 0c fe 7f 00 00  |........|.......|
; 000000b0  d8 ff c4 0c fe 7f 00 00  6b 0e c5 0c fe 7f 00 00  |........k.......|
; 000000c0  e0 ff c4 0c fe 7f 00 00  97 0e c5 0c fe 7f 00 00  |................|
; 000000d0  e8 ff c4 0c fe 7f 00 00  ae 0e c5 0c fe 7f 00 00  |................|
; 000000e0  f0 ff c4 0c fe 7f 00 00  c2 0e c5 0c fe 7f 00 00  |................|
; 000000f0  f8 ff c4 0c fe 7f 00 00  c9 0e c5 0c fe 7f 00 00  |................|
; 00000100  00 00 c5 0c fe 7f 00 00  d1 0e c5 0c fe 7f 00 00  |................|
; 00000110  08 00 c5 0c fe 7f 00 00  e2 0e c5 0c fe 7f 00 00  |................|
; 00000120  10 00 c5 0c fe 7f 00 00  01 0f c5 0c fe 7f 00 00  |................|
; 00000130  18 00 c5 0c fe 7f 00 00  1f 0f c5 0c fe 7f 00 00  |................|
; 00000140  20 00 c5 0c fe 7f 00 00  7b 0f c5 0c fe 7f 00 00  | .......{.......|
; 00000150  28 00 c5 0c fe 7f 00 00  b1 0f c5 0c fe 7f 00 00  |(...............|
; 00000160  30 00 c5 0c fe 7f 00 00  c4 0f c5 0c fe 7f 00 00  |0...............|
; 00000170  38 00 c5 0c fe 7f 00 00  de 0f c5 0c fe 7f 00 00  |8...............|
; 00000180  40 00 c5 0c fe 7f 00 00  00 00 00 00 00 00 00 00  |@...............|
; 00000190  58 58 58 58 58 58 58 58  58 58 58 58 58 58 58 58  |XXXXXXXXXXXXXXXX|
; 000001a0  80 ff c4 0c fe 7f 00 00  01 00 00 00 00 00 00 00  |................|
; 000001b0  88 ff c4 0c fe 7f 00 00  98 ff c4 0c fe 7f 00 00  |................|
; 000001c0  58 58 58 58 58 58 58 58  58 58 58 58 58 58 58 58  |XXXXXXXXXXXXXXXX|
; 000001d0
; ```

bits 64
default rel
global _start

;=============================
section .bss
;=============================

; For saving vars from exec: _start(int argc, char* argv[], char* envp[])
FrameZero: resq 1  ; Pointer to initial stack frame
ArgC: resq 1       ; Value of argc (int)
ArgV: resq 1       ; Value of argv (char* []), array length is argc
EnvP: resq 1       ; Value of envp (char* []), array is null terminated

; Data stack
align 8, db 0
%define DStackSize 16
DStackLo: resq DStackSize  ; Data stack (cell size is 64-bit qword)
DStackHi: resq 1

; String buffers
align 8, db 0
inBuf: resb 2+1022  ; Keyboard input buffer; byte 0 is length
strBuf: resb 2+1022 ; String scratch buffer; byte 0 is length


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

; Use stack as provided by exec (see System V API AMD64 supplement) both for
; C-style function calls and as return stack for subroutine threading. ABI Spec
; is at https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf, §3.4 p27.
enter_process:
mov [FrameZero], rsp     ; Save pointer to initial frame from exec
mov rdi, [rsp]           ; Save value of argc
mov [ArgC], rdi
lea rsi, [rsp+8]         ; Save argv (pointer to cstring array, length argc)
mov [ArgV], rsi
lea rcx, [rsp+8*rdi+16]  ; Save envp (pointer to cstring array, null term'ed)
mov [EnvP], rcx

; Set up stack machine's data stack which grows upward
init_data_stack:
mov edx, DStackLo
mov ecx, DStackSize   ; This assumes 1 extra qword at DStackHi
xor rsi, rsi
.for:
mov [edx+ecx], rsi
dec ecx
jnz .for
mov DSHead, 0
mov DSLen, 0

; Do the stuff...

; Note: I tried using RAX and RDX as working registers here, but they both seem
; to get corrupted when I do math with indirect address calculations. Probably
; the amd64 instruction set manual explains that somewhere. I'm guessing it's
; because a qword MUL stores its results in RDX:RAX and that mechanism gets
; used for the indirect address calculation.
copy_stack_to_strBuf:
mov rsi, rsp
lea rdi, [strBuf+2]
mov r8, 33             ; loop limit
mov rcx, r8
shl rcx, 4             ; calculate 16 * loop limit since each iteration adds
                       ; 16 bytes to the output string buffer.
mov word [rdi], cx     ; set output buffer string length
xor rcx, rcx           ; loop counter goes up
mov r9, 2              ; null counter goes down (expect 1 for argv, 1 for envp)
.for:
lea rsi, [rsp+8*rcx]   ; note: indrect address calculation stomps on RDX:RAX
shl rcx, 1             ; since I can't do `lea [rdi+16*rcx]`, prescale RCX
mov [rdi+8*rcx], rsi
mov rbp, [rsi]
mov [rdi+8*rcx+8], rbp
shr rcx, 1             ; put RCX back like it was (reverse prescale)
inc rcx                ; loop counter always gets INC'ed before any JMP
cmp rbp, 0             ; count nulls
jnz .skip
dec r9
jz .break              ; stop when null counter reaches end of envp marker
.skip:
cmp rcx, r8            ; otherwise keep going until iteration limit reached
jb .for
.break:
shl rcx, 4             ; each loop iteration added 16 bytes of output
mov word [strBuf], cx  ; set output buffer length

mPtr strBuf            ; Print the initial stack frame dump to stdout
call puts

; Print a marker that will be obvious in hexdump
mPtr dat8X
call puts

; Print the saved argc, argv, and envp values
mov rdi, strBuf
mov word [rdi], 0
mov rsi, [FrameZero]
mov word [rdi], 8*1
mov [rdi+2], rsi
mov rdx, [ArgC]
mov word [rdi], 8*2
mov [rdi+10], rdx
mov rcx, [ArgV]
mov [rdi+18], rcx
mov word [rdi], 8*3
mov r8, [EnvP]
mov [rdi+26], r8
mov word [rdi], 8*4
mPtr strBuf
call puts

; Print another marker
mPtr dat8X
call puts

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

printStack:            ; Nondestructively print stack
mov rcx, DSLen         ; loop stackDepth times
.for:
push rcx
call putln
pop rcx
dec rcx
jnz .for

putln:                 ; Write string [T] to stdout, then add a newline
call puts
mPtr datCR
call puts
ret

puts:                  ; Write string [T] to stdout
mov rax, sys_write     ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
mov rdi, stdout        ; fd
mov rsi, T             ; *buf (string starts at third byte of string record)
inc rsi
inc rsi
movzx rdx, word [T]    ; count (string length is first word of string record)
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

; Strings are 2 bytes length + n bytes data
%macro mStr 2
  dw %1
  db %2
%endmacro
datCR: mStr 1, 10
dat8X: mStr 16, "XXXXXXXXXXXXXXXX"
