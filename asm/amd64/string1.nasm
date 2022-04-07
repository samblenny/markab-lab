; This now has a pretty decent stack dumper that labels each cell of the stack.
; Since this is meant for debugging, the hexdumping routines only peek at the
; stack values without pushing or popping anything. This should reduce the
; chance of side effects due to printing stack dumps.
;
; Output from `make string1.run`:
; ```
; T  0000000000000046
; S  0000000000000045
; 03 0000000000000044
; 04 0123456789ABCDEF
; 05 0123456789ABCDEF
; 06 0000000000000043
; 07 0000000000000042
; 08 0000000000000041
; 09 00000000FFFFFFFD
; 0A 00000000FFFFFFFE
; 0B 00000000FFFFFFFF
; 0C 0000000000000009
; 0D 0000000000000008
; 0E 0000000000000007
; 0F 0000000000000006
; 10 0000000000000005
; 11 0000000000000004
; 12 0000000000000003
; Hello, world!
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
align 16, resb 0
%define DStackSize 16
DStackLo: resq DStackSize  ; Data stack (cell size is 64-bit qword)
DStackHi: resq 1

; String buffers
align 16, resb 0
%define strMax 1021
inBuf: resb 2+strMax+1   ; Keyboard input buffer; word 0 is length
strBuf: resb 2+strMax+1  ; String scratch buffer; word 0 is length
fmtBuf: resb 2+strMax+1  ; Formatter scratch buffer; word 0 is length


;=============================
section .text
;=============================

;-----------------------------
; Syscall macros

%define sys_read 0
%define sys_write 1
%define sys_exit 60
%define stdin 0
%define stdout 1

%macro alignSyscall 0  ; Syscall with manual 16-byte align of esp
  push rbp             ; preserve old rbp and rsp
  mov rbp, rsp
  and rsp, -16         ; align rsp
  syscall              ; align stack to System V ABI (maybe unnecessary?)
  mov rsp, rbp         ; restore previous rsp and rbp
  pop rbp
%endmacro

;-----------------------------
; Stack macros

%define W r10        ; Working register, full qword
%define WD r10d      ; Working register, low dword
%define WW r10w      ; Working register, low word
%define WB r10b      ; Working register, low byte
%define X r11        ; Temporary register (not preserved during CALL)

%define T r12        ; Top on stack
%define S r13        ; Second on stack
%define DSHead r14   ; Index to head of circular data stack
%define DSLen r15    ; Current depth of circular data stack (excludes T & S)

;-----------------------------
; Dictionary: macros
; These provide minor inlining

%macro mLitQ 1         ; LITQ - Push a 64-bit qword literal
  mov W, %1
  call mPush
%endmacro

%macro mLitD 1         ; LITD - Push a 32-bit dword literal with zero extend
  mov WD, %1
  call mPush
%endmacro

%macro mLitW 1         ; LITW - Push a 16-bit word literal with zero extend
  xor W, W
  mov WW, %1
  call mPush
%endmacro

%macro mLitB 1         ; LITB - Push an 8-bit byte literal with zero extend
  xor W, W
  mov WB, %1
  call mPush
%endmacro

%macro mDup 0          ; DUP - Push T
  mov W, T
  call mPush
%endmacro

%macro mSwap 0         ; SWAP - Swap T and S
  xchg T, S
%endmacro

%macro mOver 0         ; OVER - Push S
  mov W, S
  mPush
%endmacro

;-----------------------------
; Dictionary: Stack code

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

mDrop:                 ; DROP - discard (pop) T
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

;-----------------------------
; Dictionary: String code

%macro mStrLoad 1      ; Load %1 into [strBuf], replacing previous contents
  mov W, %1
  call _mStrLoad
%endmacro

%macro mStrClear 0
  mov qword [strBuf], 0
%endmacro

mStrSpace:             ; Append a space to [strBuf]
mov W, ' '
call mStrAppendByte
ret

mStrCR:                ; Append a newline to [strBuf]
mov W, 10
call mStrAppendByte
ret

mStrPutLn:             ; Write [strBuf] to stdout with a CR at the end
call mStrCR
call mStrPut
ret

_mStrLoad:             ; Overwrite [strBuf] with copy of string from [W]
lea rsi, [W+2]         ; src
mov rdi, strBuf        ; dest
movzx rdx, word [W]    ; get source string length in bytes
mov r9d, strMax
cmp rdx, r9
cmova rdx, r9          ; clip length to fit in strBuf
mov word [rdi], dx     ; store destination length
add rdi, 2             ; advance dest past length to start of string area
call _mMemcpy          ; memcpy(rdi:dest, rsi:src, rdx:lengthInBytes)
mov byte [rdi], 0      ; add null terminator for cstring compatibility
ret

mStrAppend:            ; Append string from [W] after contents of [strBuf]
push rbx
mov rdi, strBuf        ; dest
lea rsi, [W+2]         ; src
mov ebx, strMax
movzx r8, word [rdi]   ; current dest length
cmp r8, strMax         ; return if destination is full
jae .done
movzx r9, word [W]     ; get source string length in bytes
mov rdx, r8            ; calculate combined length
add rdx, r9
cmp rdx, rbx           ; clip combined length to fit in strBuf
cmova rdx, rbx
mov word [rdi], dx     ; store destination length
sub rdx, r8            ; calculate copy length (clipped source length)
add rdi, 2             ; advance dest ptr to end of current string +1
add rdi, r8
call _mMemcpy          ; memcpy(rdi:dest, rsi:src, rdx:lengthInBytes)
mov byte [rdi], 0      ; add null terminator for cstring compatibility
.done:
pop rbx
ret

mStrAppendByte:        ; Append low byte of W (raw value) after [strBuf]
mov rdi, strBuf
movzx rax, word [rdi]  ; get current string length
inc eax
cmp eax, strMax        ; return if string buffer is too full already
jae .done
mov word [rdi], ax     ; adjust string length
inc rax                ; adjust dest to start of string area (sneaky magic)
add rdi, rax
mov byte [rdi], WB     ; append the new byte
mov byte [rdi+1], 0    ; set null terminator
.done:
ret

_mMemcpy:              ; memcpy(rdi:dest, rsi:src, rdx:lengthInBytes)
mov rcx, rdx           ; save a copy of length
cld                    ; clear DF flag so MOVSx advances RSI and RDI with +1
.initialQwords:
shr rdx, 3             ; start by copying as many whole qwords as possible
cmp rdx, 0
je .finalBytes
.for1:
movsq                  ; copy qword from [rsi] to [rdi]
dec rdx
jnz .for1
.finalBytes:
mov rdx, rcx           ; finish up with remaining 0 to 7 bytes
and rdx, 7
jz .done
.for2:
movsb                  ; copy byte from [rsi] to [rdi]
dec rdx
jnz .for2
.done:
ret

;-----------------------------
; Dictionary: Formatting

mStrFmtHexQ:           ; Append W, formatted as hex digits, to [strBuf]
push rbx
mov rdi, fmtBuf
mov word [rdi], 16     ; set string length for 8 hex bytes
add rdi, 2             ; advance dest ptr to start of string data area
mov ecx, 16            ; loop for 16 hex digits because W is qword
mov rsi, W
mov r8, datFmtDigits
.for:
mov r9, rsi           ; format high nibble
shl rsi, 4            ; source shifts 1 nibble off the right
shr r9, 60            ; dest shifts 15 nibbles off the left (isolate high nib)
and r9, 0x0f
mov bl, byte [r8+r9]  ; index into the list of digits 0..F
mov byte [rdi], bl    ; add the digit to the buffer
inc rdi
dec ecx
jnz .for
mov byte [rdi], 0     ; set the cstring null terminator
mov W, fmtBuf
call mStrAppend       ; add format buffer to [strBuf]
pop rbx
ret

mStrFmtHexB:          ; Append low byte of W, formated as hex, to [strBuf]
mov rsi, datFmtDigits
mov rdi, fmtBuf
mov word [rdi], 2     ; set string length for 1 hex byte
add rdi, 2            ; advance dest ptr to start of string data area
mov X, W
shr X, 4
and X, 0x0f
mov cl, byte [rsi+X]  ; index into the list of digits 0..F
mov byte [rdi], cl    ; add the digit to the buffer
inc rdi
and W, 0x0f
mov cl, byte [rsi+W]  ; index into the list of digits 0..F
mov byte [rdi], cl    ; add the digit to the buffer
inc rdi
mov byte [rdi], 0     ; set the cstring null terminator
mov W, fmtBuf
call mStrAppend       ; add format buffer to [strBuf]
ret

%macro mDumpLabel 1   ; Append low byte of %1, then 2 spaces, to [strBuf]
  mov W, %1
  call mStrAppendByte
  call mStrSpace
  call mStrSpace
%endmacro

mDumpStack:           ; Nondestructively print hexdump of stack
push rbx              ; use rbx & rbp to preserve values across calls
push rbp
cmp DSLen, 0          ; return if stack is empty
je .done
mDumpLabel 'T'        ; format T if depth >= 1
mov W, T
call mStrFmtHexQ
call mStrCR
cmp DSLen, 1
je .done
mDumpLabel 'S'        ; format S if depth >= 2
mov W, S
call mStrFmtHexQ
call mStrCR
cmp DSLen, 2
je .done
lea rbx, [DSLen-2]    ; format the rest if depth > 2
mov rbp, DSHead
.for:
mov W, DSLen          ; print the stack depth label
sub W, rbx
inc W
call mStrFmtHexB
call mStrSpace
mov rdi, 15           ; step 1 cell down the circular data stack 
add rbp, rdi          ; equivalent to (DSHead + 16 - 1) % 16
and rbp, rdi
mov rsi, DStackLo     ; set this each time because of calls below
mov W, [rsi+8*rbp]    ; peek at the current cell's value
call mStrFmtHexQ      ; print it
call mStrCR
dec rbx
jnz .for
.done:
call mStrPut
pop rbp
pop rbx
ret

;-----------------------------
; Dictionary: Syscalls

mStrPut:               ; Write string [strBuf] to stdout, clear [strBuf]
mov rax, sys_write     ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
mov rdi, stdout        ; fd
mov rsi, strBuf        ; *buf
movzx rdx, word [rsi]  ; count (string length is first word of string record)
add rsi, 2             ; string data area starts at third byte of strBuf
alignSyscall
mov qword [strBuf], 0  ; clear [strBuf]
ret

exit:                  ; Exit process
mov rax, sys_exit      ; rax=sys_exit(rdi: code)
mov rdi, 0             ; exit code
and rsp, -16           ; align stack to System V ABI (maybe unnecessary?)
syscall


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

; Initialize buffers by setting lengths to zero (and nulling first bytes)
mov qword [inBuf], 0
mov qword [strBuf], 0
mov qword [fmtBuf], 0

; Do the stuff...

mLitB 0
mLitB 1
mLitB 2
mLitB 3
mLitB 4
mLitB 5
mLitB 6
mLitB 7
mLitB 8
mLitB 9
mLitD -1
mLitD -2
mLitD -3

mLitB 'A'
mLitB 'B'
mLitB 'C'
mLitQ 0x0123456789abcdef
mDup
mLitW 'D'
mLitD 'E'
mLitB 'F'
call mDumpStack
mStrLoad datHello
call mStrPutLn

jmp exit


;=============================
section .data
;=============================

; Strings are 2 bytes length + n bytes data + null term. The point is to allow
; for use as Forth-style length+data string or C-style data+null string.
%macro mStr 1
  %strlen %%mStrLen %1
  dw %%mStrLen
  db %1
  db 0
  align 8, db 0
%endmacro
%macro mChr 1   ; NASM strings don't do escapes like "\n", so do this instead.
  dw 1
  db %1
  db 0
  align 8, db 0
%endmacro

;-----------------------------
; Formatter data

datStrNone: mStr ""
datStrCR: mChr 10
datStrSpace: mStr " "
align 16, db 0
datFmtDigits: db "0123456789ABCDEF"

;-----------------------------
; Other data

datHello: mStr "Hello, world!"
