#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# MarkabForth code generator for shared resources between VM and kernel
#

FS_OUTFILE_TOK = "tokens.fs"
PY_OUTFILE_TOK = "tokens.py"
FS_OUTFILE_MEM = "mem_map.fs"
PY_OUTFILE_MEM = "mem_map.py"

TOKENS = """
nop Nop
& And
b@ BFetch
b! BStore
drop Drop
dup Dup
= Equal
@ Fetch
> Greater
~ Invert
io@ IOFetch
io! IOStore
< Less
- Minus
* Mul
<> NotEq
| Or
over Over
+ Plus
reset Reset
r> RFrom
<< ShiftLeft
>> ShiftRightU32
>>> ShiftRightI32
! Store
swap Swap
>r ToR
w@ WFetch
w! WStore
^ Xor
0= ZeroEq
"""

MEMORY_MAP = """
0000 IO       # Memory mapped IO area           64 bytes
003F IOEnd    # End memory mapped IO area
0040 A        # Accumulator, Address register   4 bytes
0044 T        # Top of data stack               4 bytes
0048 S        # Second on data stack            4 bytes
004C R        # top of Return stack             4 bytes
0050 IP       # Instruction Pointer             2 bytes
0052 DSDeep   # Data Stack Depth                1 byte
0053 RSDeep   # Return Stack Depth              1 byte
#...
0060 DStack   # Data Stack                      64=16*4 bytes
00A0 RStack   # Return Stack                    64=16*4 bytes
00E0 Fence    # Fence (write-protect for !)     2 bytes
#...
0100 Heap     # Heap (dictionary)               51 KB
CD00 HeapRes  # Heap Reserved buffer for WORD    1 KB
CFFF HeapMax
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

def fs_tokens():
  constants = []
  for (i, line) in enumerate(filter(TOKENS)):
    (name, opcode) = line.strip().split(" ")
    constants += [f"{i:2} const {opcode}"]
  return "\n".join(constants)

def fs_core_vocab():
  words = []
  for line in filter(TOKENS):
    (name, opcode) = line.strip().split(" ")
    words += [f": {name:5} tok> {opcode} ;"]
  return "\n".join(words)

def fs_memory_map():
  constants = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    constants += [f"{addr} const {name}"]
  return "\n".join(constants)

def py_token_for_opcode():
  kv_pairs = []
  for (i, line) in enumerate(filter(TOKENS)):
    (name, opcode) = line.strip().split(" ")
    kv_pairs += [f"  '{opcode}': {i},"]
  return "\n".join(kv_pairs)

def py_opcode_for_token():
  kv_pairs = []
  for (i, line) in enumerate(filter(TOKENS)):
    (name, opcode) = line.strip().split(" ")
    kv_pairs += [f"  {i}: '{opcode}',"]
  return "\n".join(kv_pairs)

def py_addresses():
  addrs = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    addrs += [f"{name:7} = 0x{addr}"]
  return "\n".join(addrs)

FS_TEMPLATE_TOK = f"""
( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( === THIS FILE IS AUTOMATICALLY GENERATED ===)
( ===        DO NOT MAKE EDITS HERE        ===)
( ===      See codegen.py for details      ===)

( MarkabVM virtual CPU opcode tokens)
{fs_tokens()}

( MarkabForth core vocabulary)
{fs_core_vocab()}
""".strip()

FS_TEMPLATE_MEM = f"""
( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( === THIS FILE IS AUTOMATICALLY GENERATED ===)
( ===        DO NOT MAKE EDITS HERE        ===)
( ===      See codegen.py for details      ===)

( Memory map)
( 0000..00FF belongs to VM)
( 0100..FFFF belongs to kernel)
hex
{fs_memory_map()}
decimal
""".strip()

PY_TEMPLATE_TOK = f"""
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

OPCODE_FOR_TOKEN = {{
{py_opcode_for_token()}
}}

TOKEN_FOR_OPCODE = {{
{py_token_for_opcode()}
}}

def get_opcode(token):
  return OPCODE_FOR_TOKEN[token]

def get_token(opcode):
  return TOKEN_FOR_OPCODE[opcode]
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

with open(FS_OUTFILE_TOK, 'w') as f:
  f.write(FS_TEMPLATE_TOK)
  f.write("\n")

with open(PY_OUTFILE_TOK, 'w') as f:
  f.write(PY_TEMPLATE_TOK)
  f.write("\n")

with open(FS_OUTFILE_MEM, 'w') as f:
  f.write(FS_TEMPLATE_MEM)
  f.write("\n")

with open(PY_OUTFILE_MEM, 'w') as f:
  f.write(PY_TEMPLATE_MEM)
  f.write("\n")
