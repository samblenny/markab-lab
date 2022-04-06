; Rework string handling to enable concatenation into a buffer

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

%define W r11        ; Working register

%define T r12        ; Top on stack
%define S r13        ; Second on stack
%define DSHead r14   ; Index to head of circular data stack
%define DSLen r15    ; Current depth of circular data stack (excludes T & S)

;-----------------------------
; Dictionary macros
; These provide minor inlining

%macro mPush 0         ; PUSH - Push whatever is in W
  call _mPush
%endmacro

%macro mLitQ 1         ; LITQ - Push a 64-bit qword literal
  mov W, %1
  mPush
%endmacro

%macro mLitD 1         ; LITD - Push a 32-bit dword literal with zero extend
  mov dword W, %1
  mPush
%endmacro

%macro mLitD 1         ; LITW - Push a 16-bit word literal with zero extend
  movzx word W, %1
  mPush
%endmacro

%macro mLitB 1         ; LITB - Push an 8-bit byte literal with zero extend
  movzx byte W, %1
  mPush
%endmacro

%macro mDup 0          ; DUP - Push T
  mov W, T
  mPush
%endmacro

%macro mDrop 0         ; DROP - Drop (pop) T
  call _mDrop
%endmacro

%macro mSwap 0         ; SWAP - Swap T and S
  xchg T, S
%endmacro

%macro mOver 0         ; OVER - Push S
  mov W, S
  mPush
%endmacro

%macro mStrLoad 1      ; Load %1 into [strBuf], replacing previous contents
  mov W, %1
  call _mStrLoad
%endmacro

%macro mStrAppend 1    ; Append %1 after current contents of [strBuf]
  mov W, %1
  call _mStrAppend
%endmacro

%macro mStrPut 0       ; Write [strBuf] to stdout
  call _mStrPut
%endmacro

%macro mStrPutLn 0     ; Write [strBuf] to stdout with a CR at the end
  mStrAppend datCR
  mStrPut
%endmacro

_mStrLoad:             ; Overwrite [strBuf] with copy of string from [W]
lea rsi, [W+2]         ; src
mov rdi, strBuf        ; dest
movzx rcx, word [W]    ; get source string length in bytes
mov ebx, strMax
cmp rcx, rbx
cmova rcx, rbx         ; clip length to fit in strBuf
mov word [rdi], cx     ; store destination length
add rdi, 2             ; advance dest past length to start of string area
call _mStrCopy
ret

_mStrAppend:           ; Append string from [W] after contents of [strBuf]
mov rdi, strBuf        ; dest
lea rsi, [W+2]         ; src
mov ebx, strMax
movzx r8, word [rdi]   ; current dest length
cmp r8, strMax         ; return if destination is full
jae .done
movzx r9, word [W]     ; get source string length in bytes
mov rcx, r8            ; calculate combined length
add rcx, r9
cmp rcx, rbx           ; clip combined length to fit in strBuf
cmova rcx, rbx
mov word [rdi], cx     ; store destination length
sub rcx, r8            ; calculate copy length (clipped source length)
add rdi, 2             ; advance dest ptr to end of current string +1
add rdi, r8
call _mStrCopy
.done:
ret

_mStrCopy:             ; copy(rsi:src, rdi:dest, rcx:lengthInBytes)
mov rbx, rcx           ; save a copy of length in rbx
cld                    ; clear DF flag so MOVSx advances RSI and RDI with +1
.initialQwords:
shr rcx, 3             ; start by copying as many whole qwords as possible
cmp rcx, 0
je .finalBytes
.for1:
movsq                  ; copy qword from [rsi] to [rdi]
dec rcx
jnz .for1
.finalBytes:
mov rcx, rbx           ; finish up with remaining 0 to 7 bytes
and rcx, 7
jz .done
.for2:
movsb                  ; copy byte from [rsi] to [rdi]
dec rcx
jnz .for2
.done:
mov byte [rdi], 0      ; add null terminator for cstring compatibility
ret

_mPush:                ; PUSH - Push W to data stack
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

_mDrop:                ; DROP - discard T
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

; printStack:            ; Nondestructively print stack
; mov rcx, DSLen         ; loop stackDepth times
; .for:
; push rcx
; call putln
; pop rcx
; dec rcx
; jnz .for

_mStrPut:              ; Write string [strBuf] to stdout
mov rax, sys_write     ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
mov rdi, stdout        ; fd
mov rsi, strBuf        ; *buf
movzx rdx, word [rsi]  ; count (string length is first word of string record)
add rsi, 2             ; cstring starts at third byte of strBuf
alignSyscall
mDrop
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

; Do the stuff...

mStrLoad datHello
mStrPutLn
mStrLoad datEmptyStr
mStrPutLn
mStrLoad dat16X
mStrPutLn
mStrLoad datHi
mStrPutLn
mStrLoad datLong
mStrPutLn

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
datEmptyStr: mStr ""
datCR: mChr 10
datHi: mStr "hi!"
dat16X: mStr "XXXXXXXXXXXXXXXX"
datHello: mStr "Hello, world!"
; this next one is 1020 bytes long (enough to fill strBuf once CR is appended)
datLong: mStr "This is a long line, and it's long, and it repeats itself a;sldkf 'a;sldkfaslk df;alskd jfalskdj f;laksjd f;laksjd fl;askdj f;laskdj f;laskdj f;laskdjf ;laskdj f;alskdj f;alskdj f;alskdj f;alskdj fl;askjd f alksjd flkajs d;flkj as;ldf jka;slkd jf ;laskdj f;lask jdf;lkasj df;lk asjd;lf kjas;ld kf ja;slkdj f;lask jdf;lka sjd;lf kjas;ld fkja;sl dkjf;lask jdf;lkas jdf;l kajsd;l kjafsd ;lkjfasd ;lkjafsdjlk; adsf;l kja ;jklasd ;jkla ;jklafd; jkla lk;ja ;lkja ;kljaasdf;ljkfasdk jl;asdfl jk;asdf ljk;asdf ljk;asdf; jklasdf ljk;afsd lk;fasd lk;fasd lasfd lafsd l afsdj ljk;as ljk;fasd lfasd jfasd lfasd lasfd lasfd kafsd j afsdj lk;jasdf lafsd kfasd kfasd lfasd kfsad l ;lkja jkl;af ;jklasdf; ljkfdas jkl;afsd; ljkafsd jkl;asfd ljk;asdf ;jklasdf jkl;fasd lkj;afsd ljk;fasd lasfd; fasd ;aa;sldkjfasdfas;ldkjf; lkjad ;lkjas ;lkjasd ljk;afsd klj;afd ljk;afds lj;afds kj;fads lfads k afsdl faljs lkj;a lkj;a ljk;fas lkj;fasd ljk;fasd ;fasd kfads lfdas lfads lfdas l ;klfa lkj;f lkj;f kjl;fas lkj;fads l;fads lfasd lfas j THE END"
