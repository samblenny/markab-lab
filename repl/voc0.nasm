; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; ========================================================================
; === CAUTION! This file is automatically generated. Do not edit here. ===
; ========================================================================

;----------------------------
; VM Instruction token values

%define tNop          0
%define tNext         1
%define tBye          2
%define tDup          3
%define tDrop         4
%define tSwap         5
%define tOver         6
%define tClearStack   7
%define tDotS         8
%define tDotQuoteI    9
%define tParen       10
%define tColon       11
%define tEmit        12
%define tCR          13
%define tSpace       14
%define tDot         15
%define tPlus        16
%define tMinus       17
%define tMul         18
%define tDiv         19
%define tMod         20
%define tDivMod      21
%define tMax         22
%define tMin         23
%define tAbs         24
%define tAnd         25
%define tOr          26
%define tXor         27
%define tNot         28
%define tLess        29
%define tGreater     30
%define tEqual       31
%define tZeroLess    32
%define tZeroEqual   33
%define tHex         34
%define tDecimal     35


;------------------------------------------------------------------------
; Jump table (list of dword code label addresses, indexed by token value)

align 16, db 0
db "== Jump Table =="

align 16, db 0
JumpTable:
dd mNop         ;  0
dd mNext        ;  1
dd mBye         ;  2
dd mDup         ;  3
dd mDrop        ;  4
dd mSwap        ;  5
dd mOver        ;  6
dd mClearStack  ;  7
dd mDotS        ;  8
dd mDotQuoteI   ;  9
dd mParen       ; 10
dd mColon       ; 11
dd mEmit        ; 12
dd mCR          ; 13
dd mSpace       ; 14
dd mDot         ; 15
dd mPlus        ; 16
dd mMinus       ; 17
dd mMul         ; 18
dd mDiv         ; 19
dd mMod         ; 20
dd mDivMod      ; 21
dd mMax         ; 22
dd mMin         ; 23
dd mAbs         ; 24
dd mAnd         ; 25
dd mOr          ; 26
dd mXor         ; 27
dd mNot         ; 28
dd mLess        ; 29
dd mGreater     ; 30
dd mEqual       ; 31
dd mZeroLess    ; 32
dd mZeroEqual   ; 33
dd mHex         ; 34
dd mDecimal     ; 35

%define JumpTableLen 36


;-------------------------------------------------------------
; Vocab 0 dictionary linked list (Voc0)

align 16, db 0
db "== Voc0 Dict ==="

align 16, db 0
Voc0Tail: dd 0
          db 3, "nop"
          db 1, tNop
          align 16, db 0
Voc0_001: dd Voc0Tail
          db 4, "next"
          db 1, tNext
          align 16, db 0
Voc0_002: dd Voc0_001
          db 3, "bye"
          db 1, tBye
          align 16, db 0
Voc0_003: dd Voc0_002
          db 3, "dup"
          db 1, tDup
          align 16, db 0
Voc0_004: dd Voc0_003
          db 4, "drop"
          db 1, tDrop
          align 16, db 0
Voc0_005: dd Voc0_004
          db 4, "swap"
          db 1, tSwap
          align 16, db 0
Voc0_006: dd Voc0_005
          db 4, "over"
          db 1, tOver
          align 16, db 0
Voc0_007: dd Voc0_006
          db 10, "clearstack"
          db 1, tClearStack
          align 16, db 0
Voc0_008: dd Voc0_007
          db 2, ".s"
          db 1, tDotS
          align 16, db 0
Voc0_009: dd Voc0_008
          db 2, '."'
          db 1, tDotQuoteI
          align 16, db 0
Voc0_010: dd Voc0_009
          db 1, "("
          db 1, tParen
          align 16, db 0
Voc0_011: dd Voc0_010
          db 1, ":"
          db 1, tColon
          align 16, db 0
Voc0_012: dd Voc0_011
          db 4, "emit"
          db 1, tEmit
          align 16, db 0
Voc0_013: dd Voc0_012
          db 2, "cr"
          db 1, tCR
          align 16, db 0
Voc0_014: dd Voc0_013
          db 5, "space"
          db 1, tSpace
          align 16, db 0
Voc0_015: dd Voc0_014
          db 1, "."
          db 1, tDot
          align 16, db 0
Voc0_016: dd Voc0_015
          db 1, "+"
          db 1, tPlus
          align 16, db 0
Voc0_017: dd Voc0_016
          db 1, "-"
          db 1, tMinus
          align 16, db 0
Voc0_018: dd Voc0_017
          db 1, "*"
          db 1, tMul
          align 16, db 0
Voc0_019: dd Voc0_018
          db 1, "/"
          db 1, tDiv
          align 16, db 0
Voc0_020: dd Voc0_019
          db 3, "mod"
          db 1, tMod
          align 16, db 0
Voc0_021: dd Voc0_020
          db 4, "/mod"
          db 1, tDivMod
          align 16, db 0
Voc0_022: dd Voc0_021
          db 3, "max"
          db 1, tMax
          align 16, db 0
Voc0_023: dd Voc0_022
          db 3, "min"
          db 1, tMin
          align 16, db 0
Voc0_024: dd Voc0_023
          db 3, "abs"
          db 1, tAbs
          align 16, db 0
Voc0_025: dd Voc0_024
          db 3, "and"
          db 1, tAnd
          align 16, db 0
Voc0_026: dd Voc0_025
          db 2, "or"
          db 1, tOr
          align 16, db 0
Voc0_027: dd Voc0_026
          db 3, "xor"
          db 1, tXor
          align 16, db 0
Voc0_028: dd Voc0_027
          db 3, "not"
          db 1, tNot
          align 16, db 0
Voc0_029: dd Voc0_028
          db 1, "<"
          db 1, tLess
          align 16, db 0
Voc0_030: dd Voc0_029
          db 1, ">"
          db 1, tGreater
          align 16, db 0
Voc0_031: dd Voc0_030
          db 1, "="
          db 1, tEqual
          align 16, db 0
Voc0_032: dd Voc0_031
          db 2, "0<"
          db 1, tZeroLess
          align 16, db 0
Voc0_033: dd Voc0_032
          db 2, "0="
          db 1, tZeroEqual
          align 16, db 0
Voc0_034: dd Voc0_033
          db 3, "hex"
          db 1, tHex
          align 16, db 0
Voc0Head: dd Voc0_034
          db 7, "decimal"
          db 1, tDecimal
          align 16, db 0
