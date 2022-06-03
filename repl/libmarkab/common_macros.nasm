; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; MarkabForth shared macros

bits 64
default rel

%ifndef LIBMARKAB_MACROS 
%define LIBMARKAB_MACROS

%macro mkStr 1             ; Make string with a 2-byte length prefix
  %strlen %%mStrLen %1     ; calculate length string
  dw %%mStrLen             ; 2 byte length
  db %1                    ; <length> bytes of string
%endmacro

%macro byteBuf 2
  align 16, resb 0
  %1: resb %2
%endmacro
%macro dwordBuf 2
  align 16, resb 0
  %1: resd %2
%endmacro
%macro dwordVar 1
  align 4, resb 0
  %1: resd 1
%endmacro

%define DSMax 17           ; total size of data stack (T + 16 dwords)
%define RSMax 17           ; total size of return stack (R + 16 dwords)

%define _1KB 1024
%define _2KB 2048
%define _4KB 4096
%define _8KB 8192
%define _16KB 16384
%define _32KB 32768
%define _64KB 65536

;-----------------------------
; Virtual Machine RAM Layout
;
; Glossary
; - Vocabulary: Set of words in the form of a linked list
; - Dictionary: Set of all the vocabularies
; - Virtual RAM: Zero-indexed address space available to ! and @
;

%define MemSize _64KB            ; Size of virtual Memory area

;--- Built-in Vocabularies (write protected area in Mem for builtins) ---
;
%define CoreV 0                  ; Start index of Core Vocabularies area
%define CoreVSize _1KB           ; Size of Core Vocabularies area
%define CoreVEnd CoreVSize-1     ; End index of Core Vocabularies area

%define ForthP CoreVEnd+2        ; dw Pointer to head of Forth vocabulary
%define CompileP ForthP+2        ; dw Pointer to head of Compile vocabulary
%define Context CompileP+2       ; dw Pointer to current selected vocabulary

%define BuiltinEnd Context+2-1   ; End of builtins area

%define Fence BuiltinEnd         ; Index of write protect boundary

;--- Buffers (scratch pad, terminal input, formatting, ...) ---
;
%define BuffersStart Fence+1     ; Start of buffers

%define Pad BuffersStart         ; Start of scratchPad string buffer
%define PadSize _1KB             ; Size of scratchPad
%define PadEnd Pad+PadSize-1     ; End of scratchPad

%define Fmt PadEnd+1             ; Start of right-to-left Formatting buffer
%define FmtSize _1KB             ; Size of Fmt
%define FmtEnd Fmt+FmtSize-1     ; End offset of Fmt buffer within Mem
%define FmtLI FmtEnd+1           ; dw Fmt buffer Left Index (leftmost byte)
%define FmtSgn FmtLI+2           ; db Sign of number to be formatted
%define FmtQuo FmtSgn+1          ; dd Quotient i32 (temp var for formatting)
%define FmtRem FmtQuo+4          ; dd Remainder i32 (temp var for formatting)

%define Blk FmtRem+4             ; Start of Block buffer
%define BlkSize _1KB             ; Size of Block buffer
%define BlkEnd Blk+BlkSize-1     ; End of Block buffer

%define TIB BlkEnd+1             ; Start of Terminal Input Buffer (TIB)
%define TIBSize _1KB             ; Size of TIB
%define TIBEnd TIB+TIBSize-1     ; End of TIB

%define IBPtr TIBEnd+1           ; dw Input Buffer Pointer (IBP)
%define IBLen IBPtr+2            ; dw Length of data in IBP (bytes available)
%define IN IBLen+2               ; dw Index into IBP for next unparsed byte

%define Base IN+2                ; dw Number base for number/string conversions

%define EmitBuf Base+2           ; Start of buffer for emit to use
%define EmitBufEnd EmitBuf+8     ; End of buffer for emit

%define BuffersEnd EmitBufEnd    ; End of string buffers area

;--- Heap memory for variables and compiled code ---
;
%define CallDP BuffersEnd+1      ; dw Pointer to last compiled call token
%define ColonDP CallDP+2         ; dw Pointer for temp DP saving by mColon
%define ColonLast ColonDP+2      ; dw Pointer for temp Last saving by mColon
%define DP ColonLast+2           ; dw Pointer to next free byte of Ext. Vocab
%define Last DP+2                ; dw Pointer to head of Ext. Vocab

%define Heap Last+2              ; Start of heap area
%define HReserve 260             ; Heap reserved bytes (space for mWord, etc)
%define HeapEnd MemSize-1        ; End of heap area


;-----------------------------
; Debug macro

%macro DEBUG 1
  push rax
  push rcx
  push rdx
  push rsi
  push rdi
  push r8
  push r9
  mov WB, %1
  call mEmit.W
  pop r9
  pop r8
  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rax
%endmacro


;-----------------------------
; Stack macros

%define W eax                 ; Working register, 32-bit zero-extended dword
%define WQ rax                ; Working register, 64-bit qword (for pointers)
%define WW ax                 ; Working register, 16-bit word
%define WB al                 ; Working register, low byte

%define VMBye 1               ; Bye bit: set means bye has been invoked
%define VMErr 2               ; Err bit: set means error condition
%define VMNaN 4               ; NaN bit: set means number conversion failed
%define VMCompile 8           ; Compile bit: set means compile mode is active
%define VMReturn 16           ; Return bit: set means end of outermost word
%define VMFlags r12b          ; Virtual machine status flags

%define T r13d                ; Top item of data stack (32-bits)
%define TW r13w               ; Top item of data stack (low word)
%define TB r13b               ; Top item of data stack (low byte)
%define R r15d                ; Return stack top item (32-bits)
%define DSDeep xmm0           ; Depth of data stack (includes T, NOTE! XMM0)
%define RSDeep xmm1           ; Depth of return stack (NOTE! XMM1 register)


;-----------------------------
; Pseudo-Forth macros
;
; These are for writing assembly subroutines by chaining together calls to the
; subroutines for VM instruction tokens. It's like unrolling the behavior of
; the inner interpreter. The last argument to all of these macros is a label
; to use as the jump target if there's a VM error in the token handler.
;
; This is an intermediate step towards writing features in Forth instead of
; assembly. For now, as I'm debugging the plumbing around dictionary stuff, I
; need a thing shaped like this so I can stop spending so much time and effort
; on boilerplate for working with pointers to virtual memory addresses.
;
%macro fPush 2
 mov W, %1
 call mPush
 test VMFlags, VMErr
 jnz %2
%endmacro
%macro fDo 2
 call m%[%1]
 test VMFlags, VMErr
 jnz %2
%endmacro


%endif
