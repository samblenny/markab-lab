#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Generate source code for Markab VM memory map, CPU opcodes, and enum codes
#
import re

MKB_OUTFILE = "mkb_autogen.mkb"
PY_OUTFILE = "mkb_autogen.py"

OPCODES = """
nop NOP
+ ADD
- SUB
* MUL
and AND
inv INV
or OR
xor XOR
<< SLL
>> SRL
>>> SRA
= EQ
> GT
< LT
!= NE
0= ZE
<ASM> JMP
<ASM> JAL
<ASM> RET
<ASM> BZ
<ASM> DRBLT
r> MRT
>r MTR
r R
pc PC
rdrop RDROP
drop DROP
dup DUP
over OVER
swap SWAP
<ASM> U8
<ASM> U16
<ASM> I32
@ LB
! SB
h@ LH
h! SH
w@ LW
w! SW
<ASM> RESET
iod IOD
ior IOR
iodh IODH
iorh IORH
key IOKEY
emit IOEMIT
>a MTA
@a+ LBAI
a+ AINC
a- ADEC
a A
>b MTB
!b+ SBBI
b+ BINC
b- BDEC
b B
>x MTX
x X
>Y MTY
y Y
"""

MEMORY_MAP = """
0000 Boot     # Boot code (PC=0 on reset)                768 bytes
02FF BootMax  # Boot code: last usable byte
0300 CORE_V   # Pointer to head of core vocabulary       2 bytes (align 4)
0304 FENCE    # Pointer to write protect boundary        2 bytes (align 4)
#...
0400 Heap     # Heap (dictionary)                        55 KB
E000 HeapRes  # Heap Reserve buffer for WORD             256 bytes
E0FF HeapMax  # Heap: end of reserve buffer
E100 DP       # Dictionary Pointer                       2 bytes (align 4)
E104 IN       # INput buffer index                       1 byte  (align 4)
E108 CONTEXT  # Head of vocabulary for finding words     2 bytes (align 4)
E10C CURRENT  # Head of vocabulary for new definitions   2 bytes (align 4)
E110 MODE     # Current interpreting/compiling mode      1 byte  (align 4)
E114 EXT_V    # Pointer to head of extensible vocab      2 bytes (align 4)
E11C CROSS_B  # Cross-compile Base address offset        2 bytes (align 4)
E118 CROSS_V  # Cross-compile pointer to head of Vocab   2 bytes (align 4)
#...
E200 IBLen    # Input Buffer Length             1 byte
E201 IB       # Input Buffer                    255 bytes
#E2FF            end of input buffer
E300 PadLen   # Pad buffer Length               1 byte
E301 Pad      # Pad buffer                      255 bytes
#E3FF            end of pad buffer
E400 FmtLen   # Fmt buffer Length               1 byte
E401 Fmt      # Format buffer                   255 bytes
#E4FF            end of fmt buffer
#...
FFFF MemMax
"""

ENUM_CODES = """
# Codes for dictionary entry Types
T_VAR    0   # Variable
T_CONST  1   # Constant
T_OP     2   # Single opcode for a simple word
T_OBJ    3   # Object code for regular compiled word
T_IMM    4   # Object code for immediate compiled word

# Codes for interpreter Modes
MODE_INT  0   # Interpret mode
MODE_COM  1   # Compiling mode
MODE_IMM  2   # Immediate compiling mode
"""

def filter(src):
  """Filter a comments and blank lines out of heredoc-style source string"""
  lines = src.strip().split("\n")
  lines = [L.split("#")[0].strip() for L in lines]    # filter comments
  lines = [L for L in lines if len(L) > 0]            # filter empty lines
  lines = [" ".join(re.split(r' +', L)) for L in lines] # merge repeated spaces
  return lines

def mkb_opcodes():
  constants = []
  for (i, line) in enumerate(filter(OPCODES)):
    (name, opcode) = line.strip().split(" ")
    constants += [f"{i:2} const {opcode}"]
  return "\n".join(constants)

def mkb_memory_map():
  constants = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    constants += [f"{addr} const {name}"]
  return "\n".join(constants)

def mkb_enum_codes():
  constants = []
  for line in filter(ENUM_CODES):
    (name, code) = line.split(" ")
    constants += [f"{code} const {name}"]
  return "\n".join(constants)

def py_addresses():
  addrs = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    addrs += [f"{name:7} = 0x{addr}"]
  return "\n".join(addrs)

def py_opcode_constants():
  ops = []
  for (i, line) in enumerate(filter(OPCODES)):
    (name, opcode) = line.strip().split(" ")
    ops += [f"{opcode.upper():6} = {i:2}"]
  return "\n".join(ops)

def py_enum_codes():
  constants = []
  for line in filter(ENUM_CODES):
    (name, code) = line.split(" ")
    fmt_name = f"{name}"
    constants += [f"{fmt_name:7} = {code}"]
  return "\n".join(constants)

def py_opcode_dictionary():
  ope = []
  for (i, line) in enumerate(filter(OPCODES)):
    (name, opcode) = line.strip().split(" ")
    key = f"'{opcode.upper()}':"
    ope += [f"    {key:9} {i:>2},"]
  return "\n".join(ope)

def py_core_voc():
  cv = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    key = f"'{name}':"
    cv += [f"    {key:11} (T_CONST, 0x{addr}),"]
  for line in filter(ENUM_CODES):
    (name, code) = line.split(" ")
    key = f"'{name}':"
    cv += [f"    {key:11} (T_CONST, {code}),"]
  for (i, line) in enumerate(filter(OPCODES)):
    (name, code) = line.strip().split(" ")
    if name == '<ASM>':
      continue
    fmt_name = f"'{name}':"
    key = f"'{name}':"
    cv += [f"    {key:11} (T_OP,    {code}),"]
  return "\n".join(cv)

MKB_TEMPLATE = f"""
( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( === THIS FILE IS AUTOMATICALLY GENERATED ===)
( ===        DO NOT MAKE EDITS HERE        ===)
( ===      See codegen.py for details      ===)

( Enum codes)
{mkb_enum_codes()}


( CPU opcodes)
{mkb_opcodes()}

( Memory map)
( 0000..00FF belongs to VM)
( 0100..FFFF belongs to kernel)
hex
{mkb_memory_map()}
decimal
""".strip()

PY_TEMPLATE = f"""
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

# Markab VM opcode constants
{py_opcode_constants()}

# Markab VM opcode dictionary
OPCODES = {{
{py_opcode_dictionary()}
}}

# Markab VM memory map
{py_addresses()}

# Markab language enum codes
{py_enum_codes()}

# Markab language core vocabulary
CORE_VOC = {{
{py_core_voc()}
}}
""".strip()

with open(MKB_OUTFILE, 'w') as f:
  f.write(MKB_TEMPLATE)
  f.write("\n")

with open(PY_OUTFILE, 'w') as f:
  f.write(PY_TEMPLATE)
  f.write("\n")
