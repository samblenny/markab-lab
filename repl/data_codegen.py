#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
"""
Generate assembler source code for libmarkab's data section.

This is how I bootstrap the VM token instruction set and dictionaries for
built-in words.

Dct0 struct format is:
label: dw .link             ; link to previous list entry
       db .nameLen, <name>  ; name of word
       db .type             ; type of word: TpTok TpVar TpConst TpCode
       dw .param            ; parameter: {token,immediate} | var | code
"""

OUTFILE_MACROS="libmarkab/generated_macros.nasm"
OUTFILE_DATA="libmarkab/generated_data.nasm"

# These are the type codes for the dictionary item .type field
TP_TOKEN = 0
TP_CODE = 1
TP_CONST = 2
TP_VAR = 3

# These are VM instruction token names for generating source code of token
# definitions like `%define tNext 0` and jump table links like `dd mNext`. The
# token definitions are numbers, and the jump table links correspond to labels
# of assembly language subroutines. To avoid name conflicts with assembler
# opcodes, token definitions and jump table links use `t` and `m` prefixes in
# Markab's source code.
#
TOKENS = """
Abs And ByteFetch ByteStore Call Div DivMod Drop Dup Equal Fetch FourPlus
Greater GreaterEq I I16 I32 I8 Invert Jump Less LessEq Max Min Minus Mod Mul
Nop NotEq OneMinus OnePlus Or Over Plus PopW Reset Return RFrom RPopW Store
Swap ToR TwoPlus U16 U8 WordFetch WordStore Xor ZeroEqual ZeroLess
If Else EndIf
For EndFor
Paren
Colon SemiColon Create Allot Here Last Tick
Emit CR Space DotQuoteI DotQuoteC Dot Hex Decimal
DotS DotRet DumpVars
Bye
"""

# These are names and tokens for words in markabForth's core dictionary. Names
# are spelled as Forth source code. Tokens are spelled similar to traditional
# Forth pronunciations, but using characters that are allowable for assembly
# language labels.
#
VOC0_TOKS = """
abs Abs 0
& And 0
b@ ByteFetch 0
b! ByteStore 0
/ Div 0
/% DivMod 0
drop Drop 0
dup Dup 0
= Equal 0
@ Fetch 0
4+ FourPlus 0
> Greater 0
>= GreaterEq 0
i I 0
~ Invert 0
< Less 0
<= LessEq 0
- Minus 0
% Mod 0
* Mul 0
nop Nop 0
<> NotEq 0
1- OneMinus 0
1+ OnePlus 0
| Or 0
over Over 0
+ Plus 0
reset Reset 0
r> RFrom 0
! Store 0
swap Swap 0
>r ToR 0
2+ TwoPlus 0
w@ WordFetch 0
w! WordStore 0
^ Xor 0
0= ZeroEqual 0
0< ZeroLess 0
if If -1
else Else -1
;if EndIf -1
for For -1
;for EndFor -1
( Paren -1
: Colon -1
; SemiColon -1
create Create 0
allot Allot 0
here Here 0
last Last 0
' Tick -1
emit Emit 0
cr CR 0
space Space 0
." DotQuoteI -1
. Dot 0
hex Hex 0
decimal Decimal 0
.s DotS 0
.ret DotRet 0
.vars DumpVars 0
bye Bye 0
"""

# Constants to be included in core vocabulary
VOC0_CONST = f"""
tpvar {TP_VAR}
tpconst {TP_CONST}
tpcode {TP_CODE}
tptoken {TP_TOKEN}
"""

def list_of_words(text):
  """Convert paragraph of " " and "\n" delimited words into a list of words"""
  lines = text.strip().split("\n")
  single_line = " ".join(lines)
  words = single_line.split(" ")
  return words

def make_token_defs():
  tk_list = enumerate(list_of_words(TOKENS))
  items = [f"%define t{tk:10} {i:3}" for (i, tk) in tk_list]
  return "\n".join(items)

def make_jump_table():
  tk_list = enumerate(list_of_words(TOKENS))
  items = [f"dd m{tk:10}  ; {i:2}" for (i, tk) in tk_list]
  return "\n".join(items)

def make_externs():
  items = []
  tk_list = list_of_words(TOKENS)
  items = [f"extern m{tk}" for tk in tk_list]   # `extern mFoo` jump targets
  return "\n".join(items)

def jump_table_length():
  return len(list_of_words(TOKENS))

def make_dictionary0():
  global VOC0_HEAD
  items = []
  address = 16
  link = 0
  # Add constant entries
  lines = VOC0_CONST.strip().split("\n")
  for (i, line) in enumerate(lines):
    (name, value) = line.strip().split(" ")
    fmtLink = f"dw {link}"                 # link to previous item in list
    quote = '"'
    fmtName = f"db {len(name)}, {quote}{name}{quote}"
    fmtParam = f", TpConst\ndd {value}"
    link = address
    address += 2 + 1 + len(name) + 1 + 4  # link, nameLen, <name>, type, dword
    pad_size = 0 # 16 - (address % 16)  # <- uncomment to get aligned hexdumps
    address += pad_size
    pad = ", 0" * pad_size
    items += [f"{fmtLink}\n{fmtName}{fmtParam}{pad}"]
  # Add token entries
  lines = VOC0_TOKS.strip().split("\n")
  for (i,line) in enumerate(lines):
    (name, long_name, immediate) = line.strip().split(" ")
    # Add this item to the Dct0 dictionary
    quote = "'" if ('"' in name) else '"'  # handle ." specially
    if i == len(lines) - 1:                # last item gets a link
      VOC0_HEAD = address
    token = "t" + long_name                # change long name into token macro
    fmtLink = f"dw {link}"                 # link to previous item in list
    fmtName = f"db {len(name)}, {quote}{name}{quote}"
    fmtTok = f", TpToken, {token}, {immediate}"
    link = address
    address += 2 + 1 + len(name) + 1 + 2  # link, nameLen, <name>, type, tokens
    pad_size = 0 # 16 - (address % 16)  # <- uncomment to get aligned hexdumps
    address += pad_size
    pad = ", 0" * pad_size
    items += [f"{fmtLink}\n{fmtName}{fmtTok}{pad}"]
  items += [f"Voc0End: db 0"]
  items += [f"align 16, db 0"]
  items += [f"Voc0Len: dd Voc0End - Voc0  ; {address}"]
  return "\n".join(items)

TOKEN_DEFS = make_token_defs()
JUMP_TABLE = make_jump_table()
JT_LEN = jump_table_length()
VOC0_HEAD = 0                  # this gets redefined by make_dictionary0()
VOC0 = make_dictionary0()
EXTERNS = make_externs()


TEMPLATE_MACRO = f"""
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

{TOKEN_DEFS}


;----------------------------
; Dictionary .type values

%define TpToken {TP_TOKEN}
%define TpCode  {TP_CODE}
%define TpConst {TP_CONST}
%define TpVar   {TP_VAR}


%endif
""".strip()


TEMPLATE_DATA = f"""
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

{EXTERNS}

align 16, db 0
db "== Jump Table =="

align 16, db 0
JumpTable:
{JUMP_TABLE}

%define JumpTableLen {JT_LEN}


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
{VOC0}
Voc0Head: dd {VOC0_HEAD}
""".strip()


with open(OUTFILE_DATA, 'w', encoding='utf-8') as f:
  f.write(TEMPLATE_DATA)

with open(OUTFILE_MACROS, 'w', encoding='utf-8') as f:
  f.write(TEMPLATE_MACRO)
