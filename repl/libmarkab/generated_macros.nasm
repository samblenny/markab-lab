; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; ========================================================================
; === CAUTION! This file is automatically generated. Do not edit here. ===
; ========================================================================

%ifndef LIBMARKAB_AUTOGEN_MACROS
%define LIBMARKAB_AUTOGEN_MACROS

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
%define tClearReturn  56
%define tWordStore   57
%define tWordFetch   58
%define tDumpVars    59
%define tTick        60
%define tCreate      61
%define tAllot       62
%define tHere        63
%define tLast        64
%define tOnePlus     65
%define tTwoPlus     66
%define tFourPlus    67
%define tIf          68
%define tElse        69
%define tThen        70


;----------------------------
; Dictionary .type values

%define TpToken 0
%define TpCode  1
%define TpConst 2
%define TpVar   3


%endif