; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth store and fetch words (meant to be included in ../libmarkab.nasm)

; This include path is relative to the working directory that will be in effect
; when running the Makefile in the parent directory of this file. So the
; include path is relative to ../Makefile, which is confusing.
%include "libmarkab/common_macros.nasm"

extern DSBase
extern Mem
extern mErr13AddressOOR
extern mErr1Underflow

global mFetch
global mStore
global mByteFetch
global mByteStore
global mWordFetch
global mWordStore


mFetch:                     ; Fetch: pop addr, load & push dword [Mem+T]
movq rdi, DSDeep            ; need at least 1 item on stack
cmp dil, 1
jb mErr1Underflow
test T, T                   ; check address satisfies: 0 <= T < MemSize-3
jl mErr13AddressOOR
cmp T, MemSize-3
jge mErr13AddressOOR
mov T, [Mem+T]              ; pop addr, load dword, push dword
ret

mStore:                     ; Store dword (second) at address [Mem+T]
movq rsi, DSDeep            ; make sure stack depth >= 2 items (data, address)
cmp esi, 2
jb mErr1Underflow
cmp T, Fence               ; check address satisfies: Fence < T < MemSize-3
jle mErr13AddressOOR
cmp T, MemSize-3
jge mErr13AddressOOR
mov edi, T                  ; save address
dec esi                     ; drop address
mov T, [DSBase+4*esi-4]     ; T now contains the data dword from former second
mov [Mem+edi], T            ; store data at [Mem+T]
dec rsi                     ; drop data dword
movq DSDeep, rsi
mov T, [DSBase+4*esi-4]
ret

mByteFetch:                 ; Fetch: pop addr, load & push byte [Mem+T]
movq rdi, DSDeep            ; make sure stack has at least 1 item (address)
cmp dil, 1
jb mErr1Underflow
test T, T                   ; check address satisfies: 0 <= T < MemSize
jl mErr13AddressOOR
cmp T, MemSize
jge mErr13AddressOOR
xor W, W                    ; pop addr, load dword
mov WB, byte [Mem+T]
mov T, W                    ; push dword
ret

mByteStore:                 ; Store low byte of (second) at address [Mem+T]
movq rdi, DSDeep            ; make sure stack depth >= 2 items (data, address)
cmp dil, 2
jb mErr1Underflow
cmp T, Fence                ; check address satisfies: Fence < T < MemSize
jle mErr13AddressOOR
cmp T, MemSize
jge mErr13AddressOOR
mov esi, T                  ; save address
dec edi                     ; drop address
mov T, [DSBase+4*edi-4]     ; T now contains the data from former second
mov byte [Mem+esi], TB      ; store data at [Mem+T]
dec edi                     ; drop data dword
movq DSDeep, rdi
mov T, [DSBase+4*edi-4]
ret

mWordFetch:                 ; Fetch: pop addr, load & push word [Mem+T]
movq rdi, DSDeep            ; make sure stack has at least 1 item (address)
cmp dil, 1
jb mErr1Underflow
test T, T                   ; check address satisfies: 0 <= T < MemSize-1
jl mErr13AddressOOR
cmp T, MemSize-1
jge mErr13AddressOOR
xor W, W                    ; pop addr, load dword
mov WW, word [Mem+T]
mov T, W                    ; push dword
ret

mWordStore:                 ; Store low word of (second) at address [Mem+T]
movq rdi, DSDeep            ; make sure stack depth >= 2 items (data, address)
cmp dil, 2
jb mErr1Underflow
cmp T, Fence               ; check address satisfies: Fence < T < MemSize-1
jle mErr13AddressOOR
cmp T, MemSize-1
jge mErr13AddressOOR
mov esi, T                  ; save address
dec edi                     ; drop address
mov T, [DSBase+4*edi-4]     ; T now contains the data from former second
mov word [Mem+esi], TW      ; store data at [Mem+T]
dec edi                     ; drop data dword
movq DSDeep, rdi
mov T, [DSBase+4*edi-4]
ret
