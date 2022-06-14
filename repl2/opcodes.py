# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

NOP    =  0
ADD    =  1
SUB    =  2
MUL    =  3
AND    =  4
INV    =  5
OR     =  6
XOR    =  7
SLL    =  8
SRL    =  9
SRA    = 10
EQ     = 11
GT     = 12
LT     = 13
NE     = 14
ZE     = 15
JMP    = 16
JAL    = 17
RET    = 18
BZ     = 19
DRBLT  = 20
MRT    = 21
MTR    = 22
RDROP  = 23
DROP   = 24
DUP    = 25
OVER   = 26
SWAP   = 27
U8     = 28
U16    = 29
I32    = 30
LB     = 31
SB     = 32
LH     = 33
SH     = 34
LW     = 35
SW     = 36
RESET  = 37
ECALL  = 38

E_DS    =  1
E_RS    =  2
E_DSH   =  3
E_RSH   =  4
E_PC    =  5
E_READ  =  6
E_WRITE =  7

OPCODE_ECALL = {
  'NOP':     0,
  'ADD':     1,
  'SUB':     2,
  'MUL':     3,
  'AND':     4,
  'INV':     5,
  'OR':      6,
  'XOR':     7,
  'SLL':     8,
  'SRL':     9,
  'SRA':    10,
  'EQ':     11,
  'GT':     12,
  'LT':     13,
  'NE':     14,
  'ZE':     15,
  'JMP':    16,
  'JAL':    17,
  'RET':    18,
  'BZ':     19,
  'DRBLT':  20,
  'MRT':    21,
  'MTR':    22,
  'RDROP':  23,
  'DROP':   24,
  'DUP':    25,
  'OVER':   26,
  'SWAP':   27,
  'U8':     28,
  'U16':    29,
  'I32':    30,
  'LB':     31,
  'SB':     32,
  'LH':     33,
  'SH':     34,
  'LW':     35,
  'SW':     36,
  'RESET':  37,
  'ECALL':  38,
  'E_DS':      1,
  'E_RS':      2,
  'E_DSH':     3,
  'E_RSH':     4,
  'E_PC':      5,
  'E_READ':    6,
  'E_WRITE':   7,
}
