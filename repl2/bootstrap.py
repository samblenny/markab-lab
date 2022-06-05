#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# MarkabForth bootstrap compiler
#

ROM_FILE = 'kernel.bin'

TOKENS = """
nop Nop
& And
b@ ByteFetch
b! ByteStore
/% DivMod
drop Drop
dup Dup
= Equal
@ Fetch
> Greater
~ Invert
< Less
- Minus
* Mul
<> NotEq
| Or
over Over
+ Plus
reset Reset
r> RFrom
! Store
swap Swap
>r ToR
w@ WordFetch
w! WordStore
^ Xor
0= ZeroEqual
"""

MEMORY_MAP = """
0000 IO       # Memory mapped IO area           64 bytes
003F IOEnd    # End memory mapped IO area
#... Free0    # Unallocated
0050 T        # Top of data stack               4 bytes
0054 S        # Second on data stack            4 bytes
0058 R        # top of Return stack             4 bytes
005C A        # Accumulator (address register)  4 bytes
0060 DStack   # Data Stack                      16 * 4 bytes
00A0 RStack   # Return Stack                    16 * 4 bytes
00E0 IP       # Instruction Pointer             2 bytes
00E2 IN       # INput buffer INdex              2 bytes
00E4 DP       # Dictionary Pointer              2 bytes
00E6 IBPtr    # Input Buffer Pointer            2 bytes
00E8 IBLen    # Input Buffer Length             2 bytes
00EA Fence    # Fence (write-protect for !)     2 bytes
00EC Free1    # Unallocated
#...
0400 Heap     # Heap (dictionary)               54 KB
DC00 HeapRes  # Heap Reserved area (for WORD)   1 KB
DFFF HeapMax
E000 TIB      # Terminal Input Buffer           1 KB
E3FF TIBMax
E400 BLK      # BLocK buffer                    1 KB
E7FF BLKMax
E800 Pad      # Pad buffer                      1 KB
EBFF PadMax
EC00 Fmt      # Format buffer                   1 KB
EFFF FmtMax   # End of format buffer
F000 Free2    # Unallocated
#...
FFFF MemMax
"""

def make_constants():
  constants = []
  words = []
  lines = TOKENS.strip().split("\n")
  for (i, line) in enumerate(lines):
    (name, opcode) = line.strip().split(" ")
    constants += [f"{i:2} const {opcode}"]
    words += [f": {name:5} tok> {opcode} ;"]
  result = constants
  result += words
  return "\n".join(result)

def make_memory_map():
  lines = MEMORY_MAP.strip().split("\n")
  lines = [L.split("#")[0].strip() for L in lines]    # filter comments
  lines = [L for L in lines if len(L) > 0]            # filter empty lines
  constants = ["hex"]
  for line in lines:
    (addr, name) = line.split(" ")
    constants += [f"{addr} const {name}"]
  constants += ["decimal"]
  return "\n".join(constants)

with open(ROM_FILE, 'w') as f:
  f.write("")

print("TODO: write a bootstrap compiler")

print(make_constants())
print(make_memory_map())
