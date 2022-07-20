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
reset RESET
<ASM> JMP
<ASM> JAL
<ASM> RET
<ASM> BZ
<ASM> BFOR
<ASM> U8
<ASM> U16
<ASM> I32
halt HALT
tron TRON
troff TROFF
dump IODUMP
key IOKEY
iorh IORH
load IOLOAD
save IOSAVE
>r MTR
r R
call CALL
pc PC
err ERR
>err MTE
@ LB
! SB
h@ LH
h! SH
w@ LW
w! SW
+ ADD
- SUB
* MUL
/ DIV
% MOD
<< SLL
>> SRL
>>> SRA
inv INV
xor XOR
or OR
and AND
> GT
< LT
= EQ
!= NE
0= ZE
1+ INC
1- DEC
emit IOEMIT
. IODOT
iodh IODH
iod IOD
rdrop RDROP
drop DROP
dup DUP
over OVER
swap SWAP
>a MTA
@a LBA
@a+ LBAI
a+ AINC
a- ADEC
a A
>b MTB
@b LBB
@b+ LBBI
!b+ SBBI
b+ BINC
b- BDEC
b B
true TRUE
false FALSE
"""

MEMORY_MAP = """
0000 Heap     # Heap (dictionary)                        56 KB
E000 HeapRes  # Heap Reserve buffer                      256 bytes
E0FF HeapMax  # Heap: end of reserve buffer
E100 DP       # Dictionary Pointer                       2 bytes (align 4)
E104 IN       # INput buffer index                       1 byte  (align 4)
E108 CORE_V   # Pointer to core vocab hashmap            2 bytes (align 4)
E10C EXT_V    # Pointer to extensible vocab hashmap      2 bytes (align 4)
E110 MODE     # Current interpreting/compiling mode      1 byte  (align 4)
E118 LASTCALL  # Pointer to last compiled call instr.    2 bytes (align 4)
E11C NEST     # Block Nesting level for if{ and for{     1 byte  (align 4)
E120 BASE     # Number base                              1 byte  (align 4)
E124 EOF      # Flag to indicate end of input            1 byte  (align 4)
E128 LASTWORD  # Pointer to last defined word            2 bytes (align 4)
E12C IRQRX    # IRQ vector for receiving input           2 bytes (align 4)
E130 OK_EN    # OK prompt enable                         1 byte  (align 4)
E134 LOADNEST  # IOLOAD nesting level                    1 byte  (align 4)
E138 IRQERR   # IRQ vector for error handler             2 byts  (align 4)
#...
E200 IB       # Input Buffer       256 bytes
E300 Pad      # Pad buffer         256 bytes
E400 Fmt      # Fmt buffer         256 bytes
#E4FF           end of fmt buffer
#...
FFFF MemMax
"""

CONSTANTS = """
# Codes for dictionary entry Types
T_VAR    0   # Variable
T_CONST  1   # Constant
T_OP     2   # Single opcode for a simple word
T_OBJ    3   # Object code for regular compiled word
T_IMM    4   # Object code for immediate compiled word

# Codes for interpreter Modes
MODE_INT  0   # Interpret mode
MODE_COM  1   # Compiling mode

# Error codes (most errors get set internally by the VM)
ErrUnknown 11  # Unknown word
ErrNest    12  # Compiler encountered unbalanced nesting of }if or }for

# Parameters for multiply-with-carry (mwc) string hashing function
HashA 7
HashB 8
HashC 38335
HashBins 64
HashMask 63
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
    if (name != '<ASM>') and (not opcode in ['MTR', 'RDROP']):
      continue          # skip opcodes that have a core word equivalent
    constants += [f"{i:02x} const {opcode}"]
  return "\n".join(constants)

def mkb_core_words():
  words = []
  for (i, line) in enumerate(filter(OPCODES)):
    (name, opcode) = line.strip().split(" ")
    if name == "<ASM>":
      continue
    words += [f"{i:02x} opcode {name}"]
  return "\n".join(words)

def mkb_memory_map():
  constants = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    constants += [f"{addr} const {name}"]
  return "\n".join(constants)

def mkb_enum_codes():
  constants = []
  for line in filter(CONSTANTS):
    (name, code) = line.split(" ")
    constants += [f"{int(code):02x} const {name}"]
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
  for line in filter(CONSTANTS):
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
  for line in filter(CONSTANTS):
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

hex

( Enum codes)
{mkb_enum_codes()}


( CPU opcodes)
{mkb_opcodes()}

( Core word definitions)
{mkb_core_words()}

( Memory map)
( 0000..00FF belongs to VM)
( 0100..FFFF belongs to kernel)
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
