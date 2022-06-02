; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; ========================================================================
; === CAUTION! This file is automatically generated. Do not edit here. ===
; ========================================================================
;

; The libmarkab/ prefix on these include paths is because they get resolved
; relative to the working directory of make when it is running ../Makefile.
%include "libmarkab/common_macros.nasm"
%include "libmarkab/generated_macros.nasm"

global JumpTable
global JumpTableLen
global Voc0End
global Voc0Len
global Voc0Head


;------------------------------------------------------------------------
; Jump table (list of dword code label addresses, indexed by token value)

extern mReturn
extern mNop
extern mBye
extern mDup
extern mDrop
extern mSwap
extern mOver
extern mClearStack
extern mDotS
extern mDotQuoteI
extern mParen
extern mColon
extern mEmit
extern mCR
extern mSpace
extern mDot
extern mPlus
extern mMinus
extern mMul
extern mDiv
extern mMod
extern mDivMod
extern mMax
extern mMin
extern mAbs
extern mAnd
extern mOr
extern mXor
extern mInvert
extern mLess
extern mGreater
extern mEqual
extern mZeroLess
extern mZeroEqual
extern mHex
extern mDecimal
extern mFetch
extern mStore
extern mByteFetch
extern mByteStore
extern mSemiColon
extern mDotQuoteC
extern mU8
extern mU16
extern mI8
extern mI16
extern mI32
extern mJump
extern mCall
extern mClearReturn
extern mNext
extern mNegate
extern mToR
extern mRFrom
extern mI
extern mDotRet
extern mClearReturn
extern mWordStore
extern mWordFetch
extern mDumpVars
extern mTick
extern mCreate
extern mAllot
extern mHere
extern mLast
extern mOnePlus
extern mTwoPlus
extern mFourPlus
extern mIf
extern mElse
extern mEndIf
extern mFor
extern mNext

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
dd mClearReturn  ; 56
dd mWordStore   ; 57
dd mWordFetch   ; 58
dd mDumpVars    ; 59
dd mTick        ; 60
dd mCreate      ; 61
dd mAllot       ; 62
dd mHere        ; 63
dd mLast        ; 64
dd mOnePlus     ; 65
dd mTwoPlus     ; 66
dd mFourPlus    ; 67
dd mIf          ; 68
dd mElse        ; 69
dd mEndIf       ; 70
dd mFor         ; 71
dd mNext        ; 72

%define JumpTableLen 73


;-------------------------------------------------------------
; Core vocabulary linked lists
;
; Vocabulary linked list format:
;  dw link to previous item (zero indexed, *not* section indexed)
;  db length of name, <name-bytes>, token
;
; Voc0 is meant to be copied at runtime into the core dictionary
; area of markabForth's 64KB virtual RAM memory map
;

align 16, db 0
db "== Dictionary =="

align 16, db 0
Voc0: dd 0         ; This padding is so second item's link will be non-zero
align 16, db 0
dw 0
db 5, "tpvar", TpConst
dd 3
dw 16
db 7, "tpconst", TpConst
dd 2
dw 29
db 6, "tpcode", TpConst
dd 1
dw 44
db 7, "tptoken", TpConst
dd 0
dw 58
db 3, "nop", TpToken, tNop, 0
dw 73
db 3, "bye", TpToken, tBye, 0
dw 82
db 3, "dup", TpToken, tDup, 0
dw 91
db 4, "drop", TpToken, tDrop, 0
dw 100
db 4, "swap", TpToken, tSwap, 0
dw 110
db 4, "over", TpToken, tOver, 0
dw 120
db 10, "clearstack", TpToken, tClearStack, 0
dw 130
db 2, ".s", TpToken, tDotS, 0
dw 146
db 1, "(", TpToken, tParen, -1
dw 154
db 2, '."', TpToken, tDotQuoteI, -1
dw 161
db 1, ":", TpToken, tColon, -1
dw 169
db 1, ";", TpToken, tSemiColon, -1
dw 176
db 1, "'", TpToken, tTick, -1
dw 183
db 4, "emit", TpToken, tEmit, 0
dw 190
db 2, "cr", TpToken, tCR, 0
dw 200
db 5, "space", TpToken, tSpace, 0
dw 208
db 1, ".", TpToken, tDot, 0
dw 219
db 1, "+", TpToken, tPlus, 0
dw 226
db 1, "-", TpToken, tMinus, 0
dw 233
db 6, "negate", TpToken, tNegate, 0
dw 240
db 1, "*", TpToken, tMul, 0
dw 252
db 1, "/", TpToken, tDiv, 0
dw 259
db 3, "mod", TpToken, tMod, 0
dw 266
db 4, "/mod", TpToken, tDivMod, 0
dw 275
db 3, "max", TpToken, tMax, 0
dw 285
db 3, "min", TpToken, tMin, 0
dw 294
db 3, "abs", TpToken, tAbs, 0
dw 303
db 2, "1+", TpToken, tOnePlus, 0
dw 312
db 2, "2+", TpToken, tTwoPlus, 0
dw 320
db 2, "4+", TpToken, tFourPlus, 0
dw 328
db 3, "and", TpToken, tAnd, 0
dw 336
db 2, "or", TpToken, tOr, 0
dw 345
db 3, "xor", TpToken, tXor, 0
dw 353
db 6, "invert", TpToken, tInvert, 0
dw 362
db 1, "<", TpToken, tLess, 0
dw 374
db 1, ">", TpToken, tGreater, 0
dw 381
db 1, "=", TpToken, tEqual, 0
dw 388
db 2, "0<", TpToken, tZeroLess, 0
dw 395
db 2, "0=", TpToken, tZeroEqual, 0
dw 403
db 3, "hex", TpToken, tHex, 0
dw 411
db 7, "decimal", TpToken, tDecimal, 0
dw 420
db 1, "@", TpToken, tFetch, 0
dw 433
db 1, "!", TpToken, tStore, 0
dw 440
db 2, "b@", TpToken, tByteFetch, 0
dw 447
db 2, "b!", TpToken, tByteStore, 0
dw 455
db 2, "w@", TpToken, tWordFetch, 0
dw 463
db 2, "w!", TpToken, tWordStore, 0
dw 471
db 4, "next", TpToken, tNext, 0
dw 479
db 2, ">r", TpToken, tToR, 0
dw 489
db 2, "r>", TpToken, tRFrom, 0
dw 497
db 1, "i", TpToken, tI, 0
dw 505
db 4, ".ret", TpToken, tDotRet, 0
dw 512
db 11, "clearreturn", TpToken, tClearReturn, 0
dw 522
db 5, ".vars", TpToken, tDumpVars, 0
dw 539
db 6, "create", TpToken, tCreate, 0
dw 550
db 5, "allot", TpToken, tAllot, 0
dw 562
db 4, "here", TpToken, tHere, 0
dw 573
db 4, "last", TpToken, tLast, 0
dw 583
db 2, "if", TpToken, tIf, 1
dw 593
db 4, "else", TpToken, tElse, 1
dw 601
db 5, "endif", TpToken, tEndIf, 1
dw 611
db 3, "for", TpToken, tFor, 1
dw 622
db 4, "next", TpToken, tNext, 1
Voc0End: db 0
align 16, db 0
Voc0Len: dd Voc0End - Voc0  ; 641
Voc0Head: dd 631