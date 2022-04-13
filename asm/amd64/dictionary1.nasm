; Add math operations to threaded inner interpreter.
;
; Output from `make dictionary1.run`:
; ```
; 2 1 + .S
;  T 00000003
; Drop 1 3 - .S
;  T FFFFFFFE
; Drop 3 11 * .S
;  T 00000021
; Drop 11 3 / .S
;  T 00000003
; Drop 11 3 Mod .S
;  T 00000002
; Drop 11 3 /Mod .S
;  T 00000003
; 02 00000002
; Drop Drop -11 3 /Mod .S
;  T FFFFFFFD
; 02 FFFFFFFE
; ```
;
; Partial output from `make dictionary1.dump` showing .data section:
; ```
; Hex dump of section '.data':
;   0x00402000 3d3d204a 756d7020 5461626c 65203d3d == Jump Table ==
;   0x00402010 0f114000 10114000 d4104000 dc104000 ..@...@...@...@.
;   0x00402020 e4104000 ed104000 f6104000 11114000 ..@...@...@...@.
;   0x00402030 6f114000 20114000 37114000 72134000 o.@. .@.7.@.r.@.
;   0x00402040 fe104000 fe124000 f7124000 2b134000 ..@...@...@.+.@.
;   0x00402050 ac114000 b5114000 be114000 c8114000 ..@...@...@...@.
;   0x00402060 dc114000 f0114000 1c124000 29124000 ..@...@...@.).@.
;   0x00402070 36124000 4c124000 55124000 5e124000 6.@.L.@.U.@.^.@.
;   0x00402080 67124000 75124000 8c124000 a2124000 g.@.u.@...@...@.
;   0x00402090 b8124000 d4124000 00000000 00000000 ..@...@.........
;   0x004020a0 3d3d2044 69637469 6f6e6172 79203d3d == Dictionary ==
;   0x004020b0 00000000 034e6f70 01000000 00000000 .....Nop........
;   0x004020c0 b0204000 044e6578 74010100 00000000 . @..Next.......
;   0x004020d0 c0204000 03447570 01070000 00000000 . @..Dup........
;   0x004020e0 d0204000 0444726f 70010800 00000000 . @..Drop.......
;   0x004020f0 e0204000 04537761 70010900 00000000 . @..Swap.......
;   0x00402100 f0204000 044f7665 72010a00 00000000 . @..Over.......
;   0x00402110 00214000 022e5301 0b000000 00000000 .!@...S.........
;   0x00402120 10214000 022e2201 0c000000 00000000 .!@...".........
;   0x00402130 20214000 04456d69 74010d00 00000000  !@..Emit.......
;   0x00402140 30214000 02435201 0e000000 00000000 0!@..CR.........
;   0x00402150 40214000 012e010f 00000000 00000000 @!@.............
;   0x00402160 50214000 012b0110 00000000 00000000 P!@..+..........
;   0x00402170 60214000 012d0111 00000000 00000000 `!@..-..........
;   0x00402180 70214000 012a0112 00000000 00000000 p!@..*..........
;   0x00402190 80214000 012f0113 00000000 00000000 .!@../..........
;   0x004021a0 90214000 034d6f64 01140000 00000000 .!@..Mod........
;   0x004021b0 a0214000 042f4d6f 64011500 00000000 .!@../Mod.......
;   0x004021c0 b0214000 034d6178 01160000 00000000 .!@..Max........
;   0x004021d0 c0214000 034d696e 01170000 00000000 .!@..Min........
;   0x004021e0 d0214000 03416273 01180000 00000000 .!@..Abs........
;   0x004021f0 e0214000 03416e64 01190000 00000000 .!@..And........
;   0x00402200 f0214000 024f7201 1a000000 00000000 .!@..Or.........
;   0x00402210 00224000 03586f72 011b0000 00000000 ."@..Xor........
;   0x00402220 10224000 034e6f74 011c0000 00000000 ."@..Not........
;   0x00402230 20224000 013c011d 00000000 00000000  "@..<..........
;   0x00402240 30224000 013e011e 00000000 00000000 0"@..>..........
;   0x00402250 40224000 013d011f 00000000 00000000 @"@..=..........
;   0x00402260 50224000 02303c01 20000000 00000000 P"@..0<. .......
;   0x00402270 60224000 02303d01 21000000 00000000 `"@..0=.!.......
;   0x00402280 70224000 00000000 00000000 00000000 p"@.............
;   0x00402290 3d3d204c 6f616453 63726565 6e203d3d == LoadScreen ==
;   0x004022a0 0c080032 2031202b 202e5303 02030110 ...2 1 + .S.....
;   ...
; ```

bits 64
default rel
global _start

;=============================
section .data
;=============================

;-----------------------------
; Jump table

; mkTk is a macro to make a token by doing:
; 1. Define a named token value for use in hand compiling bootstrapping words
; 2. Add code address for the token to the jump table
; 3. Increment the token value to prepare for next invocation of mkTk
; For example:
;    mkTk Foo
;    mkTk Bar
; expands to:
;    %xdefine tFoo 0
;    dw mFoo
;    %xdefine tBar 1
;    dw mBar
%assign _tkVal 0              ; set initial token value
%macro mkTk 1                 ; Make a jump table token
  %xdefine t%[%1] _tkVal      ; define named token value: `tNop`, `tU8`, ...
  dd m%[%1]                   ; add token's code address to jump table
  %assign _tkVal _tkVal+1     ; increment token value
%endmacro
%macro mkEndJumpTable 0       ; Make end of jump table bounds checking token
  %xdefine tEndJumpTable _tkVal
  %undef _tkVal
%endmacro

align 16, db 0
db "== Jump Table =="
align 16, db 0
JumpTable:
mkTk Nop
mkTk Next                     ; Next gets handled specially by doInner
mkTk U8
mkTk I8
mkTk U16
mkTk I16
mkTk I32
mkTk Dup
mkTk Drop
mkTk Swap
mkTk Over
mkTk DotS
mkTk DotQuote
mkTk Emit
mkTk CR
mkTk Dot
mkTk Plus
mkTk Minus
mkTk Mul
mkTk Div
mkTk Mod
mkTk DivMod
mkTk Max
mkTk Min
mkTk Abs
mkTk And
mkTk Or
mkTk Xor
mkTk Not
mkTk Less
mkTk Greater
mkTk Equal
mkTk ZeroLess
mkTk ZeroEqual

mkEndJumpTable                ; define tEndJumpTable for token bounds checking


;-----------------------------
; Dictionary

align 16, db 0
db "== Dictionary =="
align 16, db 0

; === Notes on Nasm Macro Syntax ===
; The macros here use some moderately fancy constructions:
; 1. Macro Parameters Range: in `%macro foo 2-*`, the `-*` means the macro
;    takes 2 or more parameters. The `%{2:-1}` matches a comma separated list
;    of parameter 2 to the last parameter. So, `foo a, b, c, ...` means that
;    `%1` expands to `a` and `%{2:-1}` expands to `b, c, ...`.
; 2. Macro Pararameter Counter: `%0` expands to the number of parameters. This
;    is useful for calculating the number of items in `%{2:-1}` for generating
;    the .tokenCount field.
; ==================================

%assign _dyN 0                ; tail of dictionary list has null link
%macro mkDyFirst 2-*          ; Make first dictionary linked list item
  %strlen %%nameLen %1        ;   length of this word's name
  %xdefine _dyThis dyN%[_dyN] ;   next item uses this label as its .link value
  align 16, db 0
  _dyThis:                    ;   relocatable label for this item
  dd 0                        ;   .link = 0
  db %%nameLen                ;   .nameLength = ...  (example:     3)
  db %1                       ;   .name = ...        (example: "Nop")
  db %0-1                     ;   .tokenCount
  db %{2:-1}                  ;   .tokenValue = ...  (example:  tNop)
%endmacro
%macro mkDyItem 2-*           ; Add entry to dictionary linked list
  %xdefine _dyPrev _dyThis    ;   save link label to previous item
  %assign _dyN _dyN+1         ;   increment dictionary item label number
  %xdefine _dyThis dyN%[_dyN] ;   make link label for this item
  %strlen %%nameLen %1        ;   length of this word's name
  align 16, db 0
  _dyThis:                    ;   relocatable label for this item
  dd _dyPrev                  ;   .link = ...        (example:  dyN0)
  db %%nameLen                ;   .nameLength = ...  (example:     3)
  db %1                       ;   .name = ...        (example: "Nop")
  db %0-1                     ;   .tokenCount
  db %{2:-1}                  ;   .tokenValue = ...  (example:  tNop)
%endmacro
%macro mkDyHead 0             ; Make dyHead pointing to head of dictionary
  align 16, db 0
  dyHead:
  dd _dyThis
  %undef _dyThis
  %undef _dyPrev
  %undef _dyN
%endmacro

mkDyFirst "Nop", tNop
mkDyItem "Next", tNext
mkDyItem "Dup", tDup
mkDyItem "Drop", tDrop
mkDyItem "Swap", tSwap
mkDyItem "Over", tOver
mkDyItem ".S", tDotS
mkDyItem '."', tDotQuote
mkDyItem "Emit", tEmit
mkDyItem "CR", tCR
mkDyItem ".", tDot
mkDyItem "+", tPlus
mkDyItem "-", tMinus
mkDyItem "*", tMul
mkDyItem "/", tDiv
mkDyItem "Mod", tMod
mkDyItem "/Mod", tDivMod
mkDyItem "Max", tMax
mkDyItem "Min", tMin
mkDyItem "Abs", tAbs
mkDyItem "And", tAnd
mkDyItem "Or", tOr
mkDyItem "Xor", tXor
mkDyItem "Not", tNot
mkDyItem "<", tLess
mkDyItem ">", tGreater
mkDyItem "=", tEqual
mkDyItem "0<", tZeroLess
mkDyItem "0=", tZeroEqual
mkDyHead


;-----------------------------
; Compiled code ROM (tokens)

%macro mkDotQuote 1           ; Compile a `." ..."` string with correct length
  %strlen %%mStrLen %1
  db tDotQuote
  dw %%mStrLen
  db %1
%endmacro

align 16, db 0
db "== LoadScreen =="
align 16, db 0
LoadScreen:                   ; Hand compiled load screen code

mkDotQuote "2 1 + .S"
db tI8, 2, tI8, 1, tPlus, tDotS
mkDotQuote "Drop 1 3 - .S"
db tDrop, tU8, 1, tU8, 3, tMinus, tDotS
mkDotQuote "Drop 3 11 * .S"
db tDrop, tU8, 3, tU8, 11, tMul, tDotS
mkDotQuote "Drop 11 3 / .S"
db tDrop, tU8, 11, tU8, 3, tDiv, tDotS
mkDotQuote "Drop 11 3 Mod .S"
db tDrop, tU8,11, tU8,3, tMod, tDotS
mkDotQuote "Drop 11 3 /Mod .S"
db tDrop, tU8,11, tU8,3, tDivMod, tDotS
mkDotQuote "Drop Drop -11 3 /Mod .S"
db tDrop, tDrop, tI8,-11, tU8,3, tDivMod, tDotS

; (CAUTION!) Token list must end with a `Next` to tell doInner that it should
; stop reading tokens. Otherwise, doInner will attempt to interpret whatever
; happens to be in memory past the end of the token list.
.end:
db tNext


;-----------------------------
; Strings {dword len, chars}

align 16, db 0
db "== VM Strings =="

datErr1se:   db 25, 0, "Error #1 Stack too empty", 10
datErr2sf:   db 24, 0, "Error #2 Stack too full", 10
datErr3btA:  db 22, 0, "Error #3 Bad token  T:"
datErr3btB:  db  4, 0, "  I:"
datErr4lt:   db 22, 0, "Error #4 Loop timeout", 10
datDotST:    db  4, 0, 10, " T "
datDotSNone: db 15, 0, "Stack is empty", 10

align 16, db 0
db "=== End.data ==="


;=============================
section .bss
;=============================

align 16, resb 0              ; Data stack
%define DSMax 17              ; total size of data stack (T + 16 dwords)
DSBase: resd DSMax-1          ; data stack (excludes T; 32-bit dword cells)

align 16, resb 0              ; Return stack (for token interpreter)
%define RSMax 16              ; total size of return stack (16 dwords)
RSBase: resd RSMax            ; data stack (32-bit dword cells)

align 16, resb 0              ; String buffers
%define StrMax 1022           ; length of string data area
TIB: resb 2+StrMax            ; terminal input buffer; word 0 is length
align 16, resb 0
Pad: resb 2+StrMax            ; string scratch buffer; word 0 is length

align 16, resb 0              ; Error message buffers
ErrToken: resd 1              ; value of current token
ErrInst: resd 1               ; instruction pointer to current token


;=============================
section .text
;=============================


;-----------------------------
; Stack macros

%define W eax                 ; Working register, 32-bit zero-extended dword
%define WQ rax                ; Working register, 64-bit qword (for pointers)
%define WB al                 ; Working register, low byte

%define T r13d                ; Top on stack, 32-bit zero-extended dword
%define TB r13b               ; Top on stack, low byte
%define DSDeep r14d           ; Current depth of data stack (including T)
%define DSDeepB r14d          ; Current depth of data stack, low byte
%define RSDeep r15d           ; Current depth of return stack
%define RSDeepB r15d          ; Current depth of return stack, low byte


;-----------------------------
; Process entry point

_start:
xor W, W                      ; init data stack registers
mov T, W
mov DSDeep, W
mov RSDeep, W
mov [TIB], W                  ; init string buffers
mov [Pad], W
.loadScreen:                  ; run the load screen
mov W, LoadScreen
call doInner
.done:
jmp mExit

;-----------------------------
; Interpreters

doInner:                      ; Inner interpreter
push rbp
push rbx
mov ebp, W                    ; ebp = instruction pointer (I)
xor ebx, ebx                  ; max loop iterations = 2^32 - 1
dec ebx
align 16                      ; align loop to a cache line
;//////////////////////////////
.for:
movzx W, byte [rbp]           ; load token at I
cmp WB, tNext                 ; handle `Next` specially
je .done
cmp WB, tEndJumpTable         ; detect token beyond jump table range (CAUTION!)
jae mErr3BadToken
lea edi, [JumpTable]          ; fetch jump table address
mov esi, dword [rdi+4*WQ]
inc ebp                       ; advance I
call rsi                      ; jump (callee may adjust I for LITx)
dec ebx
jnz .for                      ; loop until timeout (or break by NEXT token)
;//////////////////////////////
.doneTimeout:                 ; alternate exit path when loop timed out
call mErr4LoopTimeout
.done:                        ; normal exit path
pop rbx
pop rbp
ret

;-----------------------------
; Dictionary: Error handling

mErr1Underflow:               ; Handle stack too empty error
lea W, [datErr1se]            ; print error message
call mStrPut.W
xor DSDeep, DSDeep            ; clear stack
ret                           ; return control to interpreter

mErr2Overflow:                ; Handle stack too full error
lea W, [datErr2sf]            ; print error message
call mStrPut.W
xor DSDeep, DSDeep            ; clear stack
ret                           ; return control to interpreter

mErr3BadToken:                ; Handle bad token error
movzx W, byte [rbp]           ; save value of token
mov [ErrToken], W
mov W, ebp                    ; save instruction pointer to token
sub W, LoadScreen
mov [ErrInst], W
lea W, [datErr3btA]           ; print error message
call mStrPut.W
mov W, [ErrToken]             ; print token value
call mDotB.W
lea W, [datErr3btB]           ; print token instruction pointer
call mStrPut.W
mov W, [ErrInst]
lea ecx, [LoadScreen]
sub W, ecx
call mDotB.W
call mCR
jmp mExit                     ; exit

mErr4LoopTimeout:             ; Handle loop timeout error
lea W, [datErr4lt]            ; print error message
call mStrPut.W
ret                           ; return control to interpreter


;-----------------------------
; Dictionary: Literals
;
; These are designed for efficient compiling of tokens to push signed 32-bit
; numeric literals onto the stack for use with signed 32-bit math operations.
;

mU8:                          ; Push zero-extended unsigned 8-bit literal
movzx W, byte [rbp]           ; read literal from token stream
inc ebp                       ; ajust I
jmp mPush

mI8:                          ; Push sign-extended signed 8-bit literal
movsx W, byte [rbp]           ; read literal from token stream
inc ebp                       ; ajust I
jmp mPush

mU16:                         ; Push zero-extended unsigned 16-bit literal
movzx W, word [rbp]           ; read literal from token stream
add ebp, 2                    ; adjust I
jmp mPush

mI16:                         ; Push sign-extended signed 16-bit literal
movsx W, word [rbp]           ; read literal from token stream
add ebp, 2                    ; adjust I
jmp mPush

mI32:                         ; Push signed 32-bit dword literal
mov W, dword [rbp]            ; read literal from token stream
add ebp, 4                    ; adjust I
jmp mPush

mDotQuote:                    ; Print string literal to stdout
movzx ecx, word [rbp]         ; get length of string in bytes (for adjusting I)
add cx, 2                     ;   add 2 for length dword
mov W, ebp                    ; I (ebp) should be pointing to {length, chars}
add ebp, ecx                  ; adjust I past string
jmp mStrPut.W


;-----------------------------
; Dictionary: Stack ops

mNop:                         ; NOP - do nothing
ret

mNext:                        ; NEXT - (nop) this gets handled by doInner
ret

mDup:                         ; DUP - Push T
cmp DSDeepB, 1
jb mErr2Overflow
mov W, T
jmp mPush

mSwap:                        ; SWAP - Swap T and second item on stack
mov W, DSDeep
cmp WB, 2
jb mErr1Underflow
sub WB, 2
xchg T, [DSBase+4*W]
ret

mOver:                        ; OVER - Push second item on stack
mov W, DSDeep
cmp WB, 2
jb mErr1Underflow
sub WB, 2
mov W, [DSBase+4*W]
jmp mPush

mPush:                        ; PUSH - Push W to data stack
cmp DSDeep, DSMax
jnb mErr2Overflow
mov edi, W                    ; save W before relative address calculation
mov esi, DSDeep               ; calculate store index of old_depth-2+1
dec esi
mov [DSBase+4*esi], T         ; store old value of T
mov T, edi                    ; set T to caller's value of W
inc DSDeep                    ; this depth includes T + (DSMax-1) memory items
ret

mDrop:                        ; DROP - discard (pop) T
cmp DSDeep, 1
jb mErr1Underflow
dec DSDeep                    ; new_depth = old_depth-1
mov W, DSDeep                 ; new second item index = old_depth-2+1-1
dec W
mov T, [DSBase+4*W]
ret


;-----------------------------
; Dictionary: Math ops

mMathDrop:                    ; Shared drop preamble for 2-operand math ops
cmp DSDeep, 2                 ; make sure there are 2 items on the stack
jb mErr1Underflow
mov edi, T                    ; save value of old top item
dec DSDeep                    ; do a drop
mov W, DSDeep
dec W
mov T, [DSBase+4*W]
mov W, edi                    ; leave old T in eax (W) for use with math ops
ret

mPlus:                        ; +   ( 2nd T -- 2nd+T )
call mMathDrop
add T, W
ret

mMinus:                       ; -   ( 2nd T -- 2nd-T )
call mMathDrop
sub T, W
ret

mMul:                         ; *   ( 2nd T -- 2nd*T )
call mMathDrop
imul T, W                     ; imul is signed multiply (mul is unsigned)
ret

mDiv:                         ; /   ( 2nd T -- <quotient 2nd/T> )
call mMathDrop                ; after drop, old value of T is in W
cdq                           ; sign extend eax (W, old T) into rax
mov rdi, WQ                   ; save old T in rdi (use qword to prep for idiv)
mov W, T                      ; prepare dividend (old 2nd) in rax
cdq                           ; sign extend old 2nd into rax
idiv rdi                      ; signed divide 2nd/T (rax:quot, rdx:rem)
mov T, W                      ; new T is quotient from eax
ret

mMod:                         ; MOD   ( 2nd T -- <remainder 2nd/T> )
call mMathDrop                ; after drop, old value of T is in W
cdq                           ; sign extend eax (W, old T) into rax
mov rdi, WQ                   ; save old T in rdi (use qword to prep for idiv)
mov W, T                      ; prepare dividend (old 2nd) in rax
cdq                           ; sign extend old 2nd into rax
idiv rdi                      ; signed divide 2nd/T (rax:quot, rdx:rem)
mov T, edx                    ; new T is remainder from edx
ret

mDivMod:                      ; /MOD   for 2nd/T: ( 2nd T -- rem quot )
cmp DSDeep, 2                 ; make sure there are 2 items on the stack
jb mErr1Underflow
mov ecx, DSDeep               ; fetch old 2nd as dividend to eax (W)
sub cl, 2
mov W, [DSBase+4*ecx]
cdq                           ; sign extend old 2nd in eax to rax
idiv T                        ; signed divide 2nd/T (rax:quot, rdx:rem)
mov edi, eax                  ; save quotient before address calculation
mov esi, edx                  ; save remainder before address calculation
mov [DSBase+4*ecx], esi       ; remainder goes in second item
mov T, edi                    ; quotient goes in top
ret

mMax:                         ; MAX   ( 2nd T -- the_bigger_one )
call mMathDrop
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovl T, W                    ; if old 2nd was less, then use old T for new T
ret

mMin:                         ; MIN   ( 2nd T -- the_smaller_one )
call mMathDrop
cmp T, W                      ; check 2nd-T (W is old T, T is old 2nd)
cmovg T, W                    ; if old 2nd was more, then use old T for new T
ret

mAbs:                         ; ABS   ( T -- abs_of_T )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
mov W, T
neg W                         ; check if negated value of old T is positive
test W, W
cmovns T, W                   ; if so, set new T to negated old T
ret

;-----------------------------
; Dictionary: Boolean ops
;
; == Important Note! ==
; Forth's boolean constants are not like C booleans. In Forth,
;   True value:   0 (all bits clear)
;   False value: -1 (all bits set)
; This allows for sneaky tricks such as using the `AND`, `OR`, `XOR`, and `NOT`
; words to act as both bitwise and boolean operators. Also, false shows up in a
; hexdump as `FFFFFFFF`, so "F's for False" is a good mnemonic.
; =====================

mAnd:                         ; AND   ( 2nd T -- bitwise_and_2nd_T )
call mMathDrop
and T, W
ret

mOr:                          ; OR   ( 2nd T -- bitwise_or_2nd_T )
call mMathDrop
or T, W
ret

mXor:                         ; XOR   ( 2nd T -- bitwise_xor_2nd_T )
call mMathDrop
xor T, W
ret

mNot:                         ; NOT   ( T -- bitwise_negate_T )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
not T
ret

mLess:                        ; <   ( 2nd T -- bool_is_2nd_less_than_T )
call mMathDrop                ; drop leaves old T in W
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to false (-1), assuming 2nd >= T
dec T
xor esi, esi                  ; prepare true (0) in esi
cmp edi, W                    ; test for 2nd < T
cmovl T, esi                  ; if so, change new T to true
ret

mGreater:                     ; >   ( 2nd T -- bool_is_2nd_greater_than_T )
call mMathDrop                ; drop leaves old T in W
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to true (0), assuming 2nd > T
xor esi, esi                  ; prepare false (-1) in esi
dec esi
cmp edi, W                    ; test for 2nd <= T
cmovle T, esi                 ; if so, change new T to false
ret

mEqual:                       ; =   ( 2nd T -- bool_is_2nd_equal_to_T )
call mMathDrop                ; drop leaves old T in W
mov edi, T                    ; save value of old 2nd in edi
xor T, T                      ; set new T to true (0), assuming 2nd = T
xor esi, esi                  ; prepare false (-1) in esi
dec esi
cmp edi, W                    ; test for 2nd <> T   (`<>` means not-equal)
cmovnz T, esi                 ; if so, change new T to false
ret

mZeroLess:                    ; 0<   ( T -- bool_is_T_less_than_0 )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
mov W, T
xor T, T                      ; set T to false (-1)
dec T
xor edi, edi                  ; prepare value of true (0) in edi
test W, W                     ; check of old T<0 by setting sign flag (SF)
cmovs T, edi                  ; if so, change new T to true
ret

mZeroEqual:                   ; 0=   ( T -- bool_is_T_equal_0 )
cmp DSDeep, 1                 ; need at least 1 item on stack
jb mErr1Underflow
mov W, T                      ; save value of T
xor T, T                      ; set T to false (-1)
dec T
xor edi, edi                  ; prepare value of true (0) in edi
test W, W                     ; check if old T was zero (set ZF for W and W)
cmove T, edi                  ; if so, change new T to true
ret


;-----------------------------
; Dictionary: Strings

mSpace:                       ; Print a space to stdout
mov W, ' '
jmp mEmit.W

mCR:                          ; Print a CR (newline) to stdout
mov W, 10
jmp mEmit.W

mEmit:                        ; Print low byte of T as ascii char to stdout
mov W, T
call mDrop
.W:                           ; Print low byte of W as ascii char to stdout
shl W, 24                     ; Prepare W as string struct {db 1, 0, ascii}
shr W, 8
mov WB, 1
mov [dword Pad], W            ; Store string struct in Pad
jmp mStrPut


;-----------------------------
; Dictionary: Formatting

mDotB:                        ; Print T low byte to stdout (2 hex digits)
mov W, T
call mDrop
.W:                           ; Print W low byte to stdout (2 hex digits)
shl W, 24                     ; shift low byte to high end as mDot starts there
mov ecx, 2                    ; set digit count
jmp mDot.W_ecx                ; use the digit conversion loop from mDot

mDot:                         ; Print T to stdout (8 hex digits)
mov W, T
call mDrop
.W:                           ; Print W to stdout (8 hex digits)
mov ecx, 8
.W_ecx:                       ; Print ecx digits of W to stdout
lea edi, [Pad]                ; set string struct length in Pad
mov word [rdi], cx
add edi, 2                    ; advance dest ptr to start of string bytes
.for:
mov r8d, W                    ; get the high nibble of W
shr r8d, 28
shl W, 4                      ; shift that nibble off the high end of W
add r8d, '0'                  ; convert nibble assuming its value is in 0..9
mov r9d, r8d
add r9d, 'A'-'0'-10           ; but, if value was >= 10, use hex digit instead
cmp r8b, '9'
cmova r8d, r9d
mov byte [rdi], r8b           ; append digit to string bytes of [Pad]
inc edi
dec ecx
jnz .for
lea W, [Pad]                  ; print it
jmp mStrPut.W

mDotS:                        ; Nondestructively print hexdump of stack
push rbp
mov ecx, DSDeep
cmp cl, 0
je .empty
cmp cl, 1                     ; format T if data stack depth >= 1
jb .done
lea W, [datDotST]             ; T gets special label since it's not a number
call mStrPut.W
mov W, T                      ; prepare for printing T's value
xor ebp, ebp                  ; start loop counter at 1, for T's iteration
inc ebp
jmp .forPrintValue            ; for T, skip past numeric label & memory fetch
.for:
mov W, ebp
call mDotB.W                  ; print stack depth numeric label (2 is below T)
call mSpace
mov W, DSDeep                 ; fetch stack value (this gets skipped for T)
sub W, ebp
mov W, [DSBase+4*W]
.forPrintValue:               ; print the value (for both T and memory items)
call mDot.W
call mCR
inc ebp
cmp ebp, DSDeep
jbe .for                      ; loop in range 1..DSDeep
.done:                        ; clean up (normal exit point)
pop rbp
ret
.empty:                       ; alternate exit point for case of empty stack
pop rbp
lea W, [datDotSNone]
jmp mStrPut.W


;-----------------------------
; Dictionary: Syscalls

%define sys_read 0
%define sys_write 1
%define sys_exit 60
%define stdin 0
%define stdout 1

%macro alignSyscall 0         ; Syscall with manual 16-byte align of esp
  enter 0, 0                  ; preserve old rbp and rsp
  and esp, -16                ; align rsp
  syscall                     ; align stack to System V ABI
  leave                       ; restore previous rsp and rbp
%endmacro

mStrPut:                      ; Write string [Pad] to stdout, clear [Pad]
lea W, [Pad]
.W:                           ; Write string [W] to stdout, clear [W]
mov esi, W                    ; *buf (note: W is eax, so save it first)
xor eax, eax                  ; rax=sys_write(rdi: fd, rsi: *buf, rdx: count)
inc eax                       ; rax=1 means sys_write
mov edi, eax                  ; rdi=1 means fd 1, which is stdout
movzx edx, word [rsi]         ; count (string length is first word of string)
add esi, 2                    ; string data area starts at third byte of Pad
alignSyscall
ret

mExit:                        ; Exit process
mov eax, sys_exit             ; rax=sys_exit(rdi: code)
xor edi, edi                  ; exit code = 0
and esp, -16                  ; align stack to System V ABI
syscall
