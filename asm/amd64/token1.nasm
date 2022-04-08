; First attempt at a token threaded inner interpreter... seems to work okay?
;
; Output from `make token1.run`:
; ```
; T  0123456789ABCDEF
; S  0123456789ABCDEF
; 03 0000000000000043
; 04 0000000000000042
; 05 0000000000000041
; 06 00000000FFFFFFFD
; 07 00000000FFFFFFFE
; 08 00000000FFFFFFFF
; 09 0000000000000005
; 0A 0000000000000004
; 0B 0000000000000003
; 0C 0000000000000002
; 0D 0000000000000001
; Hello, world!
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
initROM:
db 0,1,  0,2,  0,3       ; 1  2  3            ( bytes)
db 1,4,0,  1,5,0         ; 4  5               ( words)
db 2,-1,-1,-1,-1         ; -1                 ( dword)
db 2,-2,-1,-1,-1         ; -2                 ( dword)
db 2,-3,-1,-1,-1         ; -3                 ( dword)
db 0,'A',  0,'B',  0,'C' ; 'A'  'B'  'C'      ( bytes)
db 3                     ; 0x0123456789abcdef ( qword)
dq 0x0123456789abcdef
db 4                     ; Dup
db 8                     ; DumpStack
db 3                     ; datHello           ( qword)
dq datHello
db 9                     ; StrLoad
db 0xb                   ; StrPutLn
db 0xC                   ; Exit
.end:


;-----------------------------
; Jump table

align 16, db 0
JumpTable:
jt00:  dw mLitB      - DictBase
jt01:  dw mLitW      - DictBase
jt02:  dw mLitD      - DictBase
jt03:  dw mLitQ      - DictBase
jt04:  dw mDup       - DictBase
jt05:  dw mDrop      - DictBase
jt06:  dw mSwap      - DictBase
jt07:  dw mOver      - DictBase
jt08:  dw mDumpStack - DictBase
jt09:  dw mStrLoad   - DictBase
jt0A:  dw mStrPut    - DictBase
jt0B:  dw mStrPutLn  - DictBase
jt0C:  dw mExit      - DictBase

jtMax: db 0x0C
align 16, db 0


;-----------------------------
; Strings

; Strings are 2 bytes length + n bytes data + null term. The point is to allow
; for use as Forth-style length+data string or C-style data+null string.
%macro mStr 1
  %strlen %%mStrLen %1
  dw %%mStrLen
  db %1
  db 0
  align 8, db 0
%endmacro

align 16, db 0
datFmtDigits: db "0123456789ABCDEF"     ; Used by number formatters

align 16, db 0
datHello: mStr "Hello, world!"
datBadToken: mStr "Bad token: "         ; Used by token interpreter


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

;-----------------------------
; Init registers & buffers

initStack:                    ; Init data stack registers
xor rax, rax
mov T, rax
mov S, rax
mov DSHead, rax
mov DSLen, rax

initStrBuf:                   ; Init string buffers
mov [inBuf], rax
mov [strBuf], rax
mov [fmtBuf], rax

;//////////////////////////////
innerInterpreter:
;//////////////////////////////
mov ebp, initROM              ; store instruction pointer (I) in rbp so
                              ;  it will be preserved during calls
align 16                      ; align loop to a fresh cache line
.while:
movzx rcx, byte [rbp]         ; load token at I
cmp cl, byte [jtMax]          ; break to debug if token is not in range
ja .errBadToken
mov edi, JumpTable            ; calculate jmp address
movzx rsi, word [rdi+2*rcx]
add rsi, DictBase
inc rbp                       ; advance I
call rsi                      ; jump (callee may adjust I for LITx)
jmp .while                    ; LOOP FOREVER! ({bad|exit} token can break)

;//////////////////////////////
.errBadToken:                 ; Exit with debug message about bad token
call mDumpStack               ; dump stack
lea W, [datBadToken]
call mStrLoad.W
movzx W, byte [rbp]           ; print offending token value
call mStrFmtHexB
call mStrCR
mov WB, 'I'                   ; print token's instruction pointer
call mDumpLabel
mov W, rbp
sub W, initROM
call mStrFmtHexB
call mStrPutLn
jmp mExit                     ; exit
;/////////////////////////////


;-----------------------------
; Dictionary base address
DictBase:


;-----------------------------
; Dictionary: Literals

mLitB:                 ; LITB - Push an 8-bit byte literal with zero extend
movzx W, byte [ebp]    ; read literal from token stream
inc ebp                ; ajust I
jmp mPush

mLitW:                 ; LITW - Push a 16-bit word literal with zero extend
movzx W, word [ebp]    ; read literal from token stream
add ebp, 2             ; adjust I
jmp mPush

mLitD:                 ; LITD - Push a 32-bit dword literal with zero extend
mov WD, dword [ebp]    ; read literal from token stream
add ebp, 4             ; adjust I
jmp mPush

mLitQ:                 ; LITQ - Push a 64-bit qword literal
mov W, qword [ebp]     ; read literal from token stream
add ebp, 8             ; adjust I
jmp mPush

;-----------------------------
; Dictionary: Stack ops

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
mov [rdx+8*DSHead], S  ; on entry, DSHead points to an availble cell
inc DSHead
and DSHead, 0x0f       ; Modulo 16 because data stack is circular
inc DSLen              ; Limit length to max capacity of data stack (16)
mov eax, 18
cmp DSLen, rax         ; Bascially, DSLen==18 probably indicates an error
cmova DSLen, rax
mov S, T
mov T, W
ret

mDrop:                 ; DROP - discard (pop) T
cmp DSLen, 0
mov T, S
mov rdx, DStackLo
mov edi, 15
add DSHead, rdi        ; Equivalent to (DSHead + 16 - 1) % 16
and DSHead, rdi
mov S, [rdx+8*DSHead]
mov rax, DSLen         ; Make sure DSLen doesn't go lower than 0
dec rax
cmovns DSLen, rax
ret

;-----------------------------
; Dictionary: Strings

mStrClear:             ; Clear [strBuf]
mov qword [strBuf], 0
ret

mStrSpace:             ; Append a space to [strBuf]
mov W, ' '
jmp mStrAppendByte

mStrCR:                ; Append a newline to [strBuf]
mov WD, 10
jmp mStrAppendByte

mStrPutLn:             ; Write [strBuf] to stdout with a CR at the end
call mStrCR
jmp mStrPut

mStrLoad:              ; Overwrite [strBuf] with copy of string from [T]
mov W, T
call mDrop             ; don't need T any more
.W:                    ; entry point for debug code that sets W itself
lea rsi, [W+2]         ; src
mov edi, strBuf        ; dest
movzx rdx, word [W]    ; get source string length in bytes
mov r9d, strMax
cmp rdx, r9
cmova rdx, r9          ; clip length to fit in strBuf
mov word [rdi], dx     ; store destination length
add rdi, 2             ; advance dest past length to start of string area
call mMemcpy           ; memcpy(rdi:dest, rsi:src, rdx:lengthInBytes)
mov byte [rdi], 0      ; add null terminator for cstring compatibility
ret

mStrAppend:            ; Append string from [W] after contents of [strBuf]
push rbx
mov edi, strBuf        ; dest
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
call mMemcpy           ; memcpy(rdi:dest, rsi:src, rdx:lengthInBytes)
mov byte [rdi], 0      ; add null terminator for cstring compatibility
.done:
pop rbx
ret

mStrAppendByte:        ; Append low byte of W (raw value) after [strBuf]
mov edi, strBuf
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

mMemcpy:              ; memcpy(rdi:dest, rsi:src, rdx:lengthInBytes)
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
mov edi, fmtBuf
mov word [rdi], 16     ; set string length for 8 hex bytes
add rdi, 2             ; advance dest ptr to start of string data area
mov ecx, 16            ; loop for 16 hex digits because W is qword
mov rsi, W
mov r8d, datFmtDigits
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
mov WD, fmtBuf
pop rbx
jmp mStrAppend        ; add format buffer to [strBuf]

mStrFmtHexB:          ; Append low byte of W, formated as hex, to [strBuf]
mov esi, datFmtDigits
mov edi, fmtBuf
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
mov WD, fmtBuf
jmp mStrAppend        ; add format buffer to [strBuf]

mDumpLabel:           ; Append low byte of %1, then 2 spaces, to [strBuf]
call mStrAppendByte
call mStrSpace
jmp mStrSpace

mDumpStack:           ; Nondestructively print hexdump of stack
push rbx              ; use rbx & rbp to preserve values across calls
push rbp
cmp DSLen, 0          ; return if stack is empty
je .done
mov WD, 'T'            ; format T if depth >= 1
call mDumpLabel
mov W, T
call mStrFmtHexQ
call mStrCR
cmp DSLen, 1
je .done
mov WD, 'S'            ; format S if depth >= 2
call mDumpLabel
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
mov edi, 15           ; step 1 cell down the circular data stack
add rbp, rdi          ; equivalent to (DSHead + 16 - 1) % 16
and rbp, rdi
mov esi, DStackLo     ; set this each time because of calls below
mov W, [rsi+8*rbp]    ; peek at the current cell's value
call mStrFmtHexQ      ; print it
call mStrCR
dec rbx
jnz .for
.done:
pop rbp
pop rbx
jmp mStrPut

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
  and rsp, -16         ; align rsp
  syscall              ; align stack to System V ABI (maybe unnecessary?)
  mov rsp, rbp         ; restore previous rsp and rbp
  pop rbp
%endmacro

mStrPut:               ; Write string [strBuf] to stdout, clear [strBuf]
mov eax, sys_write     ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
mov edi, stdout        ; fd
mov esi, strBuf        ; *buf
movzx rdx, word [rsi]  ; count (string length is first word of string record)
add rsi, 2             ; string data area starts at third byte of strBuf
alignSyscall
mov qword [strBuf], 0  ; clear [strBuf]
ret

mExit:                 ; Exit process
mov eax, sys_exit      ; rax=sys_exit(rdi: code)
mov edi, 0             ; exit code
and rsp, -16           ; align stack to System V ABI (maybe unnecessary?)
syscall
