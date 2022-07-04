# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

# Markab VM opcode constants
NOP    =  0
RESET  =  1
CLERR  =  2
JMP    =  3
JAL    =  4
RET    =  5
BZ     =  6
BFOR   =  7
U8     =  8
U16    =  9
I32    = 10
TRON   = 11
TROFF  = 12
IODUMP = 13
IOKEY  = 14
IORH   = 15
MTR    = 16
R      = 17
CALL   = 18
PC     = 19
ERR    = 20
LB     = 21
SB     = 22
LH     = 23
SH     = 24
LW     = 25
SW     = 26
ADD    = 27
SUB    = 28
MUL    = 29
DIV    = 30
MOD    = 31
SLL    = 32
SRL    = 33
SRA    = 34
INV    = 35
XOR    = 36
OR     = 37
AND    = 38
GT     = 39
LT     = 40
EQ     = 41
NE     = 42
ZE     = 43
INC    = 44
DEC    = 45
IOEMIT = 46
IODOT  = 47
IODH   = 48
IOD    = 49
RDROP  = 50
DROP   = 51
DUP    = 52
OVER   = 53
SWAP   = 54
MTA    = 55
LBA    = 56
LBAI   = 57
AINC   = 58
ADEC   = 59
A      = 60
MTB    = 61
LBB    = 62
LBBI   = 63
SBBI   = 64
BINC   = 65
BDEC   = 66
B      = 67
TRUE   = 68
FALSE  = 69

# Markab VM opcode dictionary
OPCODES = {
    'NOP':     0,
    'RESET':   1,
    'CLERR':   2,
    'JMP':     3,
    'JAL':     4,
    'RET':     5,
    'BZ':      6,
    'BFOR':    7,
    'U8':      8,
    'U16':     9,
    'I32':    10,
    'TRON':   11,
    'TROFF':  12,
    'IODUMP': 13,
    'IOKEY':  14,
    'IORH':   15,
    'MTR':    16,
    'R':      17,
    'CALL':   18,
    'PC':     19,
    'ERR':    20,
    'LB':     21,
    'SB':     22,
    'LH':     23,
    'SH':     24,
    'LW':     25,
    'SW':     26,
    'ADD':    27,
    'SUB':    28,
    'MUL':    29,
    'DIV':    30,
    'MOD':    31,
    'SLL':    32,
    'SRL':    33,
    'SRA':    34,
    'INV':    35,
    'XOR':    36,
    'OR':     37,
    'AND':    38,
    'GT':     39,
    'LT':     40,
    'EQ':     41,
    'NE':     42,
    'ZE':     43,
    'INC':    44,
    'DEC':    45,
    'IOEMIT': 46,
    'IODOT':  47,
    'IODH':   48,
    'IOD':    49,
    'RDROP':  50,
    'DROP':   51,
    'DUP':    52,
    'OVER':   53,
    'SWAP':   54,
    'MTA':    55,
    'LBA':    56,
    'LBAI':   57,
    'AINC':   58,
    'ADEC':   59,
    'A':      60,
    'MTB':    61,
    'LBB':    62,
    'LBBI':   63,
    'SBBI':   64,
    'BINC':   65,
    'BDEC':   66,
    'B':      67,
    'TRUE':   68,
    'FALSE':  69,
}

# Markab VM memory map
Heap    = 0x0000
HeapRes = 0xE000
HeapMax = 0xE0FF
DP      = 0xE100
IN      = 0xE104
CONTEXT = 0xE108
CURRENT = 0xE10C
MODE    = 0xE110
LASTCALL = 0xE118
NEST    = 0xE11C
BASE    = 0xE120
EOF     = 0xE124
IB      = 0xE200
Pad     = 0xE300
Fmt     = 0xE400
MemMax  = 0xFFFF

# Markab language enum codes
T_VAR   = 0
T_CONST = 1
T_OP    = 2
T_OBJ   = 3
T_IMM   = 4
MODE_INT = 0
MODE_COM = 1

# Markab language core vocabulary
CORE_VOC = {
    'Heap':     (T_CONST, 0x0000),
    'HeapRes':  (T_CONST, 0xE000),
    'HeapMax':  (T_CONST, 0xE0FF),
    'DP':       (T_CONST, 0xE100),
    'IN':       (T_CONST, 0xE104),
    'CONTEXT':  (T_CONST, 0xE108),
    'CURRENT':  (T_CONST, 0xE10C),
    'MODE':     (T_CONST, 0xE110),
    'LASTCALL': (T_CONST, 0xE118),
    'NEST':     (T_CONST, 0xE11C),
    'BASE':     (T_CONST, 0xE120),
    'EOF':      (T_CONST, 0xE124),
    'IB':       (T_CONST, 0xE200),
    'Pad':      (T_CONST, 0xE300),
    'Fmt':      (T_CONST, 0xE400),
    'MemMax':   (T_CONST, 0xFFFF),
    'T_VAR':    (T_CONST, 0),
    'T_CONST':  (T_CONST, 1),
    'T_OP':     (T_CONST, 2),
    'T_OBJ':    (T_CONST, 3),
    'T_IMM':    (T_CONST, 4),
    'MODE_INT': (T_CONST, 0),
    'MODE_COM': (T_CONST, 1),
    'nop':      (T_OP,    NOP),
    'reset':    (T_OP,    RESET),
    'clerr':    (T_OP,    CLERR),
    'tron':     (T_OP,    TRON),
    'troff':    (T_OP,    TROFF),
    'dump':     (T_OP,    IODUMP),
    'key':      (T_OP,    IOKEY),
    'iorh':     (T_OP,    IORH),
    '>r':       (T_OP,    MTR),
    'r':        (T_OP,    R),
    'call':     (T_OP,    CALL),
    'pc':       (T_OP,    PC),
    'err':      (T_OP,    ERR),
    '@':        (T_OP,    LB),
    '!':        (T_OP,    SB),
    'h@':       (T_OP,    LH),
    'h!':       (T_OP,    SH),
    'w@':       (T_OP,    LW),
    'w!':       (T_OP,    SW),
    '+':        (T_OP,    ADD),
    '-':        (T_OP,    SUB),
    '*':        (T_OP,    MUL),
    '/':        (T_OP,    DIV),
    '%':        (T_OP,    MOD),
    '<<':       (T_OP,    SLL),
    '>>':       (T_OP,    SRL),
    '>>>':      (T_OP,    SRA),
    'inv':      (T_OP,    INV),
    'xor':      (T_OP,    XOR),
    'or':       (T_OP,    OR),
    'and':      (T_OP,    AND),
    '>':        (T_OP,    GT),
    '<':        (T_OP,    LT),
    '=':        (T_OP,    EQ),
    '!=':       (T_OP,    NE),
    '0=':       (T_OP,    ZE),
    '1+':       (T_OP,    INC),
    '1-':       (T_OP,    DEC),
    'emit':     (T_OP,    IOEMIT),
    '.':        (T_OP,    IODOT),
    'iodh':     (T_OP,    IODH),
    'iod':      (T_OP,    IOD),
    'rdrop':    (T_OP,    RDROP),
    'drop':     (T_OP,    DROP),
    'dup':      (T_OP,    DUP),
    'over':     (T_OP,    OVER),
    'swap':     (T_OP,    SWAP),
    '>a':       (T_OP,    MTA),
    '@a':       (T_OP,    LBA),
    '@a+':      (T_OP,    LBAI),
    'a+':       (T_OP,    AINC),
    'a-':       (T_OP,    ADEC),
    'a':        (T_OP,    A),
    '>b':       (T_OP,    MTB),
    '@b':       (T_OP,    LBB),
    '@b+':      (T_OP,    LBBI),
    '!b+':      (T_OP,    SBBI),
    'b+':       (T_OP,    BINC),
    'b-':       (T_OP,    BDEC),
    'b':        (T_OP,    B),
    'true':     (T_OP,    TRUE),
    'false':    (T_OP,    FALSE),
}
