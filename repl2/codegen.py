#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Generate source code for Markab VM memory map, CPU opcodes, and ECALL codes
#

MKB_OUTFILE_OP = "opcodes.mkb"
MKB_OUTFILE_MEM = "mem_map.mkb"
PY_OUTFILE_OP = "opcodes.py"
PY_OUTFILE_MEM = "mem_map.py"

OPCODES = """
nop NOP
+ ADD
- SUB
* MUL
& AND
~ INV
| OR
^ XOR
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
; RET
<ASM> BZ
<ASM> DRBLT
r> MRT
>r MTR
rdrop RDROP
drop DROP
dup DUP
over OVER
swap SWAP
<ASM> U8
<ASM> U16
<ASM> I32
b@ LB
b! SB
h@ LH
h! SH
w@ LW
w! SW
reset RESET
ecall ECALL
"""

ECALLS = """
1 E_DS
2 E_RS
3 E_DSH
4 E_RSH
5 E_PC
6 E_READ
7 E_WRITE
"""

MEMORY_MAP = """
0000 IO       # Memory mapped IO area           64 bytes
00FF IOEnd    # End memory mapped IO area
#...
0100 Boot     # Boot code (IP=Boot on reset)    768 bytes
03FF BootMax
#...
0400 Heap     # Heap (dictionary)               50 KB
CC00 HeapRes  # Heap Reserved buffer for WORD   1 KB
CFFF HeapMax
#...
D000 DP       # Dictionary Pointer              2 bytes (align 4)
D004 IN       # INput buffer INdex              2 bytes (align 4)
D008 IBPtr    # Input Buffer Pointer            2 bytes (align 4)
D00C IBLen    # Input Buffer Length             2 bytes (align 4)
#...
E000 TIB      # Terminal Input Buffer           1 KB
E3FF TIBMax
E400 BLK      # BLocK buffer                    1 KB
E7FF BLKMax
E800 Pad      # Pad buffer                      1 KB
EBFF PadMax
EC00 Fmt      # Format buffer                   1 KB
EFFF FmtMax   # End of format buffer
#...
FFFF MemMax
"""

def filter(src):
  """Filter a comments and blank lines out of heredoc-style source string"""
  lines = src.strip().split("\n")
  lines = [L.split("#")[0].strip() for L in lines]    # filter comments
  lines = [L for L in lines if len(L) > 0]            # filter empty lines
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

def mkb_ecall_constants():
  ecalls = []
  for (i, line) in enumerate(filter(ECALLS)):
    (code, name) = line.strip().split(" ")
    ecalls += [f"{code:>2} const {name}"]
  return "\n".join(ecalls)

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

def py_ecall_constants():
  ecalls = []
  for (i, line) in enumerate(filter(ECALLS)):
    (code, name) = line.strip().split(" ")
    ecalls += [f"{name:7} = {code:>2}"]
  return "\n".join(ecalls)

MKB_TEMPLATE_OP = f"""
( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( === THIS FILE IS AUTOMATICALLY GENERATED ===)
( ===        DO NOT MAKE EDITS HERE        ===)
( ===      See codegen.py for details      ===)

( CPU opcodes)
{mkb_opcodes()}

( Environment call (ECALL) constants)
{mkb_ecall_constants()}
""".strip()

MKB_TEMPLATE_MEM = f"""
( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( === THIS FILE IS AUTOMATICALLY GENERATED ===)
( ===        DO NOT MAKE EDITS HERE        ===)
( ===      See codegen.py for details      ===)

( Memory map)
( 0000..00FF belongs to VM)
( 0100..FFFF belongs to kernel)
hex
{mkb_memory_map()}
decimal
""".strip()

PY_TEMPLATE_OP = f"""
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

{py_opcode_constants()}

{py_ecall_constants()}
""".strip()

PY_TEMPLATE_MEM = f"""
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

{py_addresses()}
""".strip()

with open(MKB_OUTFILE_OP, 'w') as f:
  f.write(MKB_TEMPLATE_OP)
  f.write("\n")

with open(MKB_OUTFILE_MEM, 'w') as f:
  f.write(MKB_TEMPLATE_MEM)
  f.write("\n")

with open(PY_OUTFILE_OP, 'w') as f:
  f.write(PY_TEMPLATE_OP)
  f.write("\n")

with open(PY_OUTFILE_MEM, 'w') as f:
  f.write(PY_TEMPLATE_MEM)
  f.write("\n")
