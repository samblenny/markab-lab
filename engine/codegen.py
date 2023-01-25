#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Generate source code for Markab VM memory map, CPU opcodes, and enum codes
#
import re
from os.path import basename


MKB_OUTFILE = "mkb_autogen.mkb"
C_HEADER_OUTFILE = "libmkb/autogen.h"
C_CODE_OUTFILE = "libmkb/autogen.c"

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
load_ IOLOAD
fopen_ FOPEN
fread FREAD
fwrite FWRITE
fseek FSEEK
ftell FTELL
ftrunc FTRUNC
fclose FCLOSE
>r MTR
r R
call CALL
pc PC
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
E118 LASTCALL # Pointer to last compiled call instr.     2 bytes (align 4)
E11C NEST     # Block Nesting level for if{ and for{     1 byte  (align 4)
E120 BASE     # Number base                              1 byte  (align 4)
E124 EOF      # Flag to indicate end of input            1 byte  (align 4)
E128 LASTWORD # Pointer to last defined word             2 bytes (align 4)
E12C IRQRX    # IRQ vector for receiving input           2 bytes (align 4)
E130 OK_EN    # OK prompt enable                         1 byte  (align 4)
E134 LOADNEST # IOLOAD nesting level                     1 byte  (align 4)
E138 IRQERR   # IRQ vector for error handler             2 bytes (align 4)
#...
E200 IB       # Input Buffer       256 bytes
E300 Pad      # Pad buffer         256 bytes
E400 Scratch  # Scratch buffer     256 bytes
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
ErrFilepath 9  # Filepath error while opening file

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

# ========================================================

def c_addresses():
  addrs = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    addrs += [f"#define MK_{name:9} (0x{addr})"]
  return "\n".join(addrs)

def c_opcode_constants():
  ops = []
  for (i, line) in enumerate(filter(OPCODES)):
    (name, opcode) = line.strip().split(" ")
    ops += [f"#define MK_{opcode.upper():6} (0x{i:02x}  /* {i:2} */)"]
  return "\n".join(ops)

def c_enum_codes():
  constants = []
  for line in filter(CONSTANTS):
    (name, code) = line.split(" ")
    fmt_name = f"{name}"
    constants += [f"#define MK_{fmt_name:11} ({code})"]
  return "\n".join(constants)

def c_opcode_dictionary():
  ope = []
  for (i, line) in enumerate(filter(OPCODES)):
    (name, opcode) = line.strip().split(" ")
    key = f'"{opcode.upper()}",'
    ope += [f"\t{key:9}  // {i:>2}"]
  return "\n".join(ope)

def c_core_vocab_len():
  a = len(filter(MEMORY_MAP))
  b = len(filter(CONSTANTS))
  c = len(filter(OPCODES))
  return a + b + c

def c_opcodes_len():
  return len(filter(OPCODES))

def c_core_vocab():
  cv = []
  for line in filter(MEMORY_MAP):
    (addr, name) = line.split(" ")
    key = f"{{\"{name}\"}},"
    cv += [f"""\t{{ {key:16} MK_T_CONST, 0x{addr:7} }},"""]
  for line in filter(CONSTANTS):
    (name, code) = line.split(" ")
    key = f"{{\"{name}\"}},"
    cv += [f"""\t{{ {key:16} MK_T_CONST, {code:9} }},"""]
  for (i, line) in enumerate(filter(OPCODES)):
    (name, code) = line.strip().split(" ")
    if name == '<ASM>':
      continue
    key = f"{{\"{name}\"}},"
    cv += [f"""\t{{ {key:16} MK_T_OP,    MK_{code:6} }},"""]
  return "\n".join(cv)

def c_bytecode_switch_guts():
  s = []
  for (i, line) in enumerate(filter(OPCODES)):
    (name, opcode) = line.strip().split(" ")
    s += [f"\t\t\tcase {i}:"]
    s += [f"\t\t\t\top_{opcode.upper()}(ctx);"]
    s += [f"\t\t\t\tbreak;"]
  s += ["\t\t\tdefault:"]
  s += ["\t\t\t\tvm_irq_err(ctx, MK_ERR_BAD_INSTRUCTION);"]
  return "\n".join(s)

def c_op_h():
  s = []
  for line in filter(OPCODES):
    (name, opcode) = line.strip().split(" ")
    s += [f"static void op_{opcode.upper()}(mk_context_t * ctx);\n"]
  return "\n".join(s)

def c_op_c():
  s = []
  for line in filter(OPCODES):
    (name, opcode) = line.strip().split(" ")
    s += [f"// {opcode}  ( -- ) "]
    s += [f"static void op_{opcode.upper()}(mk_context_t * ctx) {{\n}}\n"]
  return "\n".join(s)


# ========================================================

MKB_TEMPLATE = f"""
( Copyright © 2023 Sam Blenny)
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

C_HEADER_TEMPLATE = f"""
// Copyright (c) 2023 Sam Blenny
// SPDX-License-Identifier: MIT
//
// THIS FILE IS AUTOMATICALLY GENERATED
// DO NOT MAKE EDITS HERE
// See codegen.py for details
//
#ifndef LIBMKB_AUTOGEN_H
#define LIBMKB_AUTOGEN_H

#include <stdint.h>

// Shorthand integer typedefs to save on typing
typedef  uint8_t  u8;
typedef   int8_t  i8;
typedef uint16_t u16;
typedef  int16_t i16;
typedef  int32_t i32;
typedef uint32_t u32;

// Markab VM opcode constants
{c_opcode_constants()}

// Markab VM opcode dictionary
#define MK_OPCODES_LEN ({c_opcodes_len()})
static const char * const opcodes[MK_OPCODES_LEN];

// Markab VM memory map
{c_addresses()}

// Markab language enum codes
{c_enum_codes()}

// Markab language core vocabulary
#define MK_CORE_VOC_LEN ({c_core_vocab_len()})
#define MK_VOC_ITEM_NAME_LEN (16)
typedef struct mk_voc_item {{
\tconst char * const name[MK_VOC_ITEM_NAME_LEN];
\tconst uint8_t type_code;
\tconst u32 value;
}} mk_voc_item_t;
static const mk_voc_item_t mk_core_voc[MK_CORE_VOC_LEN];

// VM context struct for holding state of registers and RAM
typedef struct mk_context {{
\ti32 err;               // Error register (don't confuse with ERR opcode!)
\tu8  base;              // number Base for debug printing
\ti32 A;                 // register for source address or scratch
\ti32 B;                 // register for destination addr or scratch
\ti32 T;                 // Top of data stack
\ti32 S;                 // Second on data stack
\ti32 R;                 // top of Return stack
\tu16 PC;                // Program Counter
\tu8  DSDeep;            // Data Stack Depth (count include T and S)
\tu32 RSDeep;            // Return Stack Depth (count inlcudes R)
\tu32 DStack[16];        // Data Stack
\tu32 RStack[16];        // Return Stack
\tu8  RAM[MK_MemMax+1];  // Random Access Memory
\tu8  InBuf[256];        // Input buffer
\tu8  OutBuf[256];       // Output buffer
\tu8  echo;              // Echo depends on tty vs pip, etc.
\tu8  halted;            // Flag to track halt (used for `bye`)
\tu8  HoldStdout;        // Flag to use holding buffer for stdout
\tu8  IOLOAD_depth;      // Nesting level for io_load_file()
\tu8  IOLOAD_fail;       // Flag indicating an error during io_load_file()
\tu8  FOPEN_file;        // File (if any) that was opened by FOPEN 
\tu8  DbgTraceEnable;    // Debug trace on/off
}} mk_context_t;

// Maximum number of cycles allowed before infinite loop error triggers
#define MK_MAX_CYCLES (65535)

static void autogen_step(mk_context_t * ctx);

#endif /* LIBMKB_AUTOGEN_H */
""".strip()

C_CODE_TEMPLATE = f"""
// Copyright (c) 2023 Sam Blenny
// SPDX-License-Identifier: MIT
//
// THIS FILE IS AUTOMATICALLY GENERATED
// DO NOT MAKE EDITS HERE
// See codegen.py for details
//
#ifndef LIBMKB_AUTOGEN_C
#define LIBMKB_AUTOGEN_C

#include "{basename(C_HEADER_OUTFILE)}"

// Markab VM opcode dictionary
static const char * const opcodes[MK_OPCODES_LEN] = {{
{c_opcode_dictionary()}
}};

// Markab language core vocabulary
static const mk_voc_item_t core_voc[MK_CORE_VOC_LEN] = {{
{c_core_vocab()}
}};

/*
 * This is the bytecode interpreter. The for-loop here is a very, very hot code
 * path, so we need to be careful to help the compiler optimize it well. With
 * that in mind, this code expects to be #included into libmkb.c, which also
 * #includes op.c. That arrangement allows the compiler to inline opcode
 * implementations into the big switch statement.
 */
static void autogen_step(mk_context_t * ctx) {{
\tfor(int i=0; i<MK_MAX_CYCLES; i++) {{
\t\tswitch(vm_next_instruction(ctx)) {{
{c_bytecode_switch_guts()}
\t\t}};
\t\tif(ctx->halted) {{
\t\t\treturn;
\t\t}}
\t}}
\t// Making it this far means the MK_MAX_CYCLES limit was exceeded
\tvm_irq_err(ctx, MK_ERR_MAX_CYCLES);
\tautogen_step(ctx);
}};

#endif /* LIBMKB_AUTOGEN_C */
""".strip()

with open(MKB_OUTFILE, 'w') as f:
  f.write(MKB_TEMPLATE)
  f.write("\n")

with open(C_HEADER_OUTFILE, 'w') as f:
  f.write(C_HEADER_TEMPLATE)
  f.write("\n")

with open(C_CODE_OUTFILE, 'w') as f:
  f.write(C_CODE_TEMPLATE)
  f.write("\n")

# # These generate boilerplate for all the opcode implementation functions
# with open("libmkb/op.c.temp", 'w') as f:
#   f.write(c_op_c())
#   f.write("\n")
# with open("libmkb/op.h.temp", 'w') as f:
#   f.write(c_op_h())
#   f.write("\n")
