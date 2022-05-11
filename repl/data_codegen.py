#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
"""
Generate assembler source code for libmarkab's data section.

This is how I bootstrap the VM token instruction set and dictionaries for
built-in words.

Dct0 struct format is:
label: dd .link             ; link to previous list entry
       db .nameLen, <name>  ; name of word
       db .wordType         ; type of word: 0:token, 1:var, 2:code
       (db|dw) .param       ; parameter: (db token|dw varPtr|dw codePtr)
"""

# These are VM instruction token names for generating source code of token
# definitions like `%define tNext 0` and jump table links like `dd mNext`. The
# token definitions are numbers, and the jump table links correspond to labels
# of assembly language subroutines. To avoid name conflicts with assembler
# opcodes, token definitions and jump table links use `t` and `m` prefixes in
# Markab's source code.
#
TOKENS = """
Next Nop Bye Dup Drop Swap Over ClearStack DotS DotQuoteI Paren Colon Emit
CR Space Dot Plus Minus Mul Div Mod DivMod Max Min Abs And Or Xor Not Less
Greater Equal ZeroLess ZeroEqual Hex Decimal Fetch Store ByteFetch ByteStore
SemiColon DotQuoteC
"""

# These are names and tokens for words in Markab Forth's core dictionary
# (Dct0). Names are spelled as Forth source code. Tokens are spelled out
# similar to traditional Forth pronunciations, using characters that are
# allowable for assembly language labels.
#
# The main difference between DCT0_LIST and TOKENS is that DCT0_LIST does not
# include Next. This is because Next is meant to be used as machine code for
# returning from the end of a compiled word.
#
DCT0_LIST = """
nop Nop
bye Bye
dup Dup
drop Drop
swap Swap
over Over
clearstack ClearStack
.s DotS
." DotQuoteI
( Paren
: Colon
emit Emit
cr CR
space Space
. Dot
+ Plus
- Minus
* Mul
/ Div
mod Mod
/mod DivMod
max Max
min Min
abs Abs
and And
or Or
xor Xor
not Not
< Less
> Greater
= Equal
0< ZeroLess
0= ZeroEqual
hex Hex
decimal Decimal
@ Fetch
! Store
b@ ByteFetch
b! ByteStore
; SemiColon
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

def jump_table_length():
  return len(list_of_words(TOKENS))

def make_dictionary0():
  items = []
  serial = 0
  label = "Dct0Tail"  # first item gets a special label
  link = "0"
  lines = DCT0_LIST.strip().split("\n")
  for (i,line) in enumerate(lines):
    (name, long_name) = line.strip().split(" ")
    # Add this item to the Dct0 dictionary
    quote = "'" if ('"' in name) else '"'  # handle ." specially
    if i == len(lines) - 1:                # last item gets special label
      label = "Dct0Head"
    indent = " " * (len(label)+2)
    token = "t" + long_name                # change long name into token macro
    fmtLink = f"{label}: dd {link}"        # link to previous item in list
    fmtName = f"{indent}db {len(name)}, {quote}{name}{quote}"
    fmtToks = f"{indent}db 0, {token}"
    items += [f"{fmtLink}\n{fmtName}\n{fmtToks}\n{indent}align 16, db 0"]
    serial += 1
    link = label
    label = f"Dct0_{serial:03}"
  return "\n".join(items)

TOKEN_DEFS = make_token_defs()
JUMP_TABLE = make_jump_table()
JT_LEN = jump_table_length()
DCT0 = make_dictionary0()

TEMPLATE = f"""
; Copyright (c) 2022 Sam Blenny
; SPDX-License-Identifier: MIT
;
; ========================================================================
; === CAUTION! This file is automatically generated. Do not edit here. ===
; ========================================================================

;----------------------------
; VM Instruction token values

{TOKEN_DEFS}


;------------------------------------------------------------------------
; Jump table (list of dword code label addresses, indexed by token value)

align 16, db 0
db "== Jump Table =="

align 16, db 0
JumpTable:
{JUMP_TABLE}

%define JumpTableLen {JT_LEN}


;-------------------------------------------------------------
; Dictionary linked list (Dct0)

align 16, db 0
db "== Dictionary =="

align 16, db 0
{DCT0}
Dct0End: db 0
""".strip()

print(TEMPLATE)
