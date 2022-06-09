# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

OPCODE_FOR_TOKEN = {
  0: 'NOP',
  1: 'ADD',
  2: 'SUB',
  3: 'MUL',
  4: 'AND',
  5: 'INV',
  6: 'OR',
  7: 'XOR',
  8: 'SLL',
  9: 'SRL',
  10: 'SRA',
  11: 'EQ',
  12: 'GT',
  13: 'LT',
  14: 'NE',
  15: 'ZE',
  16: 'JMP',
  17: 'JAL',
  18: 'RET',
  19: 'BZ',
  20: 'DRBLT',
  21: 'MRT',
  22: 'MTR',
  23: 'DROP',
  24: 'DUP',
  25: 'OVER',
  26: 'SWAP',
  27: 'U8',
  28: 'U16',
  29: 'I32',
  30: 'LB',
  31: 'SB',
  32: 'LH',
  33: 'SH',
  34: 'LW',
  35: 'SW',
  36: 'RESET',
  37: 'BREAK',
}

TOKEN_FOR_OPCODE = {
  'NOP': 0,
  'ADD': 1,
  'SUB': 2,
  'MUL': 3,
  'AND': 4,
  'INV': 5,
  'OR': 6,
  'XOR': 7,
  'SLL': 8,
  'SRL': 9,
  'SRA': 10,
  'EQ': 11,
  'GT': 12,
  'LT': 13,
  'NE': 14,
  'ZE': 15,
  'JMP': 16,
  'JAL': 17,
  'RET': 18,
  'BZ': 19,
  'DRBLT': 20,
  'MRT': 21,
  'MTR': 22,
  'DROP': 23,
  'DUP': 24,
  'OVER': 25,
  'SWAP': 26,
  'U8': 27,
  'U16': 28,
  'I32': 29,
  'LB': 30,
  'SB': 31,
  'LH': 32,
  'SH': 33,
  'LW': 34,
  'SW': 35,
  'RESET': 36,
  'BREAK': 37,
}

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
DROP   = 23
DUP    = 24
OVER   = 25
SWAP   = 26
U8     = 27
U16    = 28
I32    = 29
LB     = 30
SB     = 31
LH     = 32
SH     = 33
LW     = 34
SW     = 35
RESET  = 36
BREAK  = 37

def get_opcode(token):
  return OPCODE_FOR_TOKEN[token]

def get_token(opcode):
  return TOKEN_FOR_OPCODE[opcode]
