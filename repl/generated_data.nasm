; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; ========================================================================
; === CAUTION! This file is automatically generated. Do not edit here. ===
; ========================================================================

;----------------------------
; VM Instruction token values

%define tReturn       0
%define tNop          1
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
%define tInvert      28
%define tLess        29
%define tGreater     30
%define tEqual       31
%define tZeroLess    32
%define tZeroEqual   33
%define tHex         34
%define tDecimal     35
%define tFetch       36
%define tStore       37
%define tByteFetch   38
%define tByteStore   39
%define tSemiColon   40
%define tDotQuoteC   41
%define tU8          42
%define tU16         43
%define tI8          44
%define tI16         45
%define tI32         46
%define tJump        47
%define tCall        48
%define tClearReturn  49
%define tNext        50
%define tNegate      51
%define tToR         52
%define tRFrom       53
%define tI           54
%define tDotRet      55


;------------------------------------------------------------------------
; Jump table (list of dword code label addresses, indexed by token value)

align 16, db 0
db "== Jump Table =="

align 16, db 0
JumpTable:
dd mReturn      ;  0
dd mNop         ;  1
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
dd mInvert      ; 28
dd mLess        ; 29
dd mGreater     ; 30
dd mEqual       ; 31
dd mZeroLess    ; 32
dd mZeroEqual   ; 33
dd mHex         ; 34
dd mDecimal     ; 35
dd mFetch       ; 36
dd mStore       ; 37
dd mByteFetch   ; 38
dd mByteStore   ; 39
dd mSemiColon   ; 40
dd mDotQuoteC   ; 41
dd mU8          ; 42
dd mU16         ; 43
dd mI8          ; 44
dd mI16         ; 45
dd mI32         ; 46
dd mJump        ; 47
dd mCall        ; 48
dd mClearReturn  ; 49
dd mNext        ; 50
dd mNegate      ; 51
dd mToR         ; 52
dd mRFrom       ; 53
dd mI           ; 54
dd mDotRet      ; 55

%define JumpTableLen 56


;-------------------------------------------------------------
; Dictionary linked list (Dct0)

align 16, db 0
db "== Dictionary =="

align 16, db 0
Dct0Tail: dd 0
          db 3, "nop"
          db 0, tNop, tReturn
          align 16, db 0
Dct0_001: dd Dct0Tail
          db 3, "bye"
          db 0, tBye, tReturn
          align 16, db 0
Dct0_002: dd Dct0_001
          db 3, "dup"
          db 0, tDup, tReturn
          align 16, db 0
Dct0_003: dd Dct0_002
          db 4, "drop"
          db 0, tDrop, tReturn
          align 16, db 0
Dct0_004: dd Dct0_003
          db 4, "swap"
          db 0, tSwap, tReturn
          align 16, db 0
Dct0_005: dd Dct0_004
          db 4, "over"
          db 0, tOver, tReturn
          align 16, db 0
Dct0_006: dd Dct0_005
          db 10, "clearstack"
          db 0, tClearStack, tReturn
          align 16, db 0
Dct0_007: dd Dct0_006
          db 2, ".s"
          db 0, tDotS, tReturn
          align 16, db 0
Dct0_008: dd Dct0_007
          db 2, '."'
          db 0, tDotQuoteI, tReturn
          align 16, db 0
Dct0_009: dd Dct0_008
          db 1, "("
          db 0, tParen, tReturn
          align 16, db 0
Dct0_010: dd Dct0_009
          db 1, ":"
          db 0, tColon, tReturn
          align 16, db 0
Dct0_011: dd Dct0_010
          db 4, "emit"
          db 0, tEmit, tReturn
          align 16, db 0
Dct0_012: dd Dct0_011
          db 2, "cr"
          db 0, tCR, tReturn
          align 16, db 0
Dct0_013: dd Dct0_012
          db 5, "space"
          db 0, tSpace, tReturn
          align 16, db 0
Dct0_014: dd Dct0_013
          db 1, "."
          db 0, tDot, tReturn
          align 16, db 0
Dct0_015: dd Dct0_014
          db 1, "+"
          db 0, tPlus, tReturn
          align 16, db 0
Dct0_016: dd Dct0_015
          db 1, "-"
          db 0, tMinus, tReturn
          align 16, db 0
Dct0_017: dd Dct0_016
          db 6, "negate"
          db 0, tNegate, tReturn
          align 16, db 0
Dct0_018: dd Dct0_017
          db 1, "*"
          db 0, tMul, tReturn
          align 16, db 0
Dct0_019: dd Dct0_018
          db 1, "/"
          db 0, tDiv, tReturn
          align 16, db 0
Dct0_020: dd Dct0_019
          db 3, "mod"
          db 0, tMod, tReturn
          align 16, db 0
Dct0_021: dd Dct0_020
          db 4, "/mod"
          db 0, tDivMod, tReturn
          align 16, db 0
Dct0_022: dd Dct0_021
          db 3, "max"
          db 0, tMax, tReturn
          align 16, db 0
Dct0_023: dd Dct0_022
          db 3, "min"
          db 0, tMin, tReturn
          align 16, db 0
Dct0_024: dd Dct0_023
          db 3, "abs"
          db 0, tAbs, tReturn
          align 16, db 0
Dct0_025: dd Dct0_024
          db 3, "and"
          db 0, tAnd, tReturn
          align 16, db 0
Dct0_026: dd Dct0_025
          db 2, "or"
          db 0, tOr, tReturn
          align 16, db 0
Dct0_027: dd Dct0_026
          db 3, "xor"
          db 0, tXor, tReturn
          align 16, db 0
Dct0_028: dd Dct0_027
          db 6, "invert"
          db 0, tInvert, tReturn
          align 16, db 0
Dct0_029: dd Dct0_028
          db 1, "<"
          db 0, tLess, tReturn
          align 16, db 0
Dct0_030: dd Dct0_029
          db 1, ">"
          db 0, tGreater, tReturn
          align 16, db 0
Dct0_031: dd Dct0_030
          db 1, "="
          db 0, tEqual, tReturn
          align 16, db 0
Dct0_032: dd Dct0_031
          db 2, "0<"
          db 0, tZeroLess, tReturn
          align 16, db 0
Dct0_033: dd Dct0_032
          db 2, "0="
          db 0, tZeroEqual, tReturn
          align 16, db 0
Dct0_034: dd Dct0_033
          db 3, "hex"
          db 0, tHex, tReturn
          align 16, db 0
Dct0_035: dd Dct0_034
          db 7, "decimal"
          db 0, tDecimal, tReturn
          align 16, db 0
Dct0_036: dd Dct0_035
          db 1, "@"
          db 0, tFetch, tReturn
          align 16, db 0
Dct0_037: dd Dct0_036
          db 1, "!"
          db 0, tStore, tReturn
          align 16, db 0
Dct0_038: dd Dct0_037
          db 2, "b@"
          db 0, tByteFetch, tReturn
          align 16, db 0
Dct0_039: dd Dct0_038
          db 2, "b!"
          db 0, tByteStore, tReturn
          align 16, db 0
Dct0_040: dd Dct0_039
          db 1, ";"
          db 0, tSemiColon, tReturn
          align 16, db 0
Dct0_041: dd Dct0_040
          db 4, "next"
          db 0, tNext, tReturn
          align 16, db 0
Dct0_042: dd Dct0_041
          db 2, ">r"
          db 0, tToR, tReturn
          align 16, db 0
Dct0_043: dd Dct0_042
          db 2, "r>"
          db 0, tRFrom, tReturn
          align 16, db 0
Dct0_044: dd Dct0_043
          db 1, "i"
          db 0, tI, tReturn
          align 16, db 0
Dct0Head: dd Dct0_044
          db 4, ".ret"
          db 0, tDotRet, tReturn
          align 16, db 0
Dct0End: db 0
