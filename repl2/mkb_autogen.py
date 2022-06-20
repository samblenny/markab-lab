# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

# Markab VM opcode constants
NOP    =  0
ADD    =  1
SUB    =  2
INC    =  3
DEC    =  4
MUL    =  5
AND    =  6
INV    =  7
OR     =  8
XOR    =  9
SLL    = 10
SRL    = 11
SRA    = 12
EQ     = 13
GT     = 14
LT     = 15
NE     = 16
ZE     = 17
TRUE   = 18
FALSE  = 19
JMP    = 20
JAL    = 21
RET    = 22
BZ     = 23
DRBLT  = 24
MRT    = 25
MTR    = 26
R      = 27
PC     = 28
RDROP  = 29
DROP   = 30
DUP    = 31
OVER   = 32
SWAP   = 33
U8     = 34
U16    = 35
I32    = 36
LB     = 37
SB     = 38
LH     = 39
SH     = 40
LW     = 41
SW     = 42
RESET  = 43
IOD    = 44
IOR    = 45
IODH   = 46
IORH   = 47
IOKEY  = 48
IOEMIT = 49
MTA    = 50
LBA    = 51
LBAI   = 52
AINC   = 53
ADEC   = 54
A      = 55
MTB    = 56
LBB    = 57
LBBI   = 58
SBBI   = 59
BINC   = 60
BDEC   = 61
B      = 62
MTX    = 63
X      = 64
MTY    = 65
Y      = 66

# Markab VM opcode dictionary
OPCODES = {
    'NOP':     0,
    'ADD':     1,
    'SUB':     2,
    'INC':     3,
    'DEC':     4,
    'MUL':     5,
    'AND':     6,
    'INV':     7,
    'OR':      8,
    'XOR':     9,
    'SLL':    10,
    'SRL':    11,
    'SRA':    12,
    'EQ':     13,
    'GT':     14,
    'LT':     15,
    'NE':     16,
    'ZE':     17,
    'TRUE':   18,
    'FALSE':  19,
    'JMP':    20,
    'JAL':    21,
    'RET':    22,
    'BZ':     23,
    'DRBLT':  24,
    'MRT':    25,
    'MTR':    26,
    'R':      27,
    'PC':     28,
    'RDROP':  29,
    'DROP':   30,
    'DUP':    31,
    'OVER':   32,
    'SWAP':   33,
    'U8':     34,
    'U16':    35,
    'I32':    36,
    'LB':     37,
    'SB':     38,
    'LH':     39,
    'SH':     40,
    'LW':     41,
    'SW':     42,
    'RESET':  43,
    'IOD':    44,
    'IOR':    45,
    'IODH':   46,
    'IORH':   47,
    'IOKEY':  48,
    'IOEMIT': 49,
    'MTA':    50,
    'LBA':    51,
    'LBAI':   52,
    'AINC':   53,
    'ADEC':   54,
    'A':      55,
    'MTB':    56,
    'LBB':    57,
    'LBBI':   58,
    'SBBI':   59,
    'BINC':   60,
    'BDEC':   61,
    'B':      62,
    'MTX':    63,
    'X':      64,
    'MTY':    65,
    'Y':      66,
}

# Markab VM memory map
Boot    = 0x0000
BootMax = 0x02FF
CORE_V  = 0x0300
FENCE   = 0x0304
Heap    = 0x0400
HeapRes = 0xE000
HeapMax = 0xE0FF
DP      = 0xE100
IN      = 0xE104
CONTEXT = 0xE108
CURRENT = 0xE10C
MODE    = 0xE110
EXT_V   = 0xE114
LASTCALL = 0xE118
IBLen   = 0xE200
IB      = 0xE201
PadLen  = 0xE300
Pad     = 0xE301
FmtLen  = 0xE400
Fmt     = 0xE401
MemMax  = 0xFFFF

# Markab language enum codes
T_VAR   = 0
T_CONST = 1
T_OP    = 2
T_OBJ   = 3
T_IMM   = 4
MODE_INT = 0
MODE_COM = 1
MODE_IMM = 2

# Markab language core vocabulary
CORE_VOC = {
    'Boot':     (T_CONST, 0x0000),
    'BootMax':  (T_CONST, 0x02FF),
    'CORE_V':   (T_CONST, 0x0300),
    'FENCE':    (T_CONST, 0x0304),
    'Heap':     (T_CONST, 0x0400),
    'HeapRes':  (T_CONST, 0xE000),
    'HeapMax':  (T_CONST, 0xE0FF),
    'DP':       (T_CONST, 0xE100),
    'IN':       (T_CONST, 0xE104),
    'CONTEXT':  (T_CONST, 0xE108),
    'CURRENT':  (T_CONST, 0xE10C),
    'MODE':     (T_CONST, 0xE110),
    'EXT_V':    (T_CONST, 0xE114),
    'LASTCALL': (T_CONST, 0xE118),
    'IBLen':    (T_CONST, 0xE200),
    'IB':       (T_CONST, 0xE201),
    'PadLen':   (T_CONST, 0xE300),
    'Pad':      (T_CONST, 0xE301),
    'FmtLen':   (T_CONST, 0xE400),
    'Fmt':      (T_CONST, 0xE401),
    'MemMax':   (T_CONST, 0xFFFF),
    'T_VAR':    (T_CONST, 0),
    'T_CONST':  (T_CONST, 1),
    'T_OP':     (T_CONST, 2),
    'T_OBJ':    (T_CONST, 3),
    'T_IMM':    (T_CONST, 4),
    'MODE_INT': (T_CONST, 0),
    'MODE_COM': (T_CONST, 1),
    'MODE_IMM': (T_CONST, 2),
    'nop':      (T_OP,    NOP),
    '+':        (T_OP,    ADD),
    '-':        (T_OP,    SUB),
    '1+':       (T_OP,    INC),
    '1-':       (T_OP,    DEC),
    '*':        (T_OP,    MUL),
    'and':      (T_OP,    AND),
    'inv':      (T_OP,    INV),
    'or':       (T_OP,    OR),
    'xor':      (T_OP,    XOR),
    '<<':       (T_OP,    SLL),
    '>>':       (T_OP,    SRL),
    '>>>':      (T_OP,    SRA),
    '=':        (T_OP,    EQ),
    '>':        (T_OP,    GT),
    '<':        (T_OP,    LT),
    '!=':       (T_OP,    NE),
    '0=':       (T_OP,    ZE),
    'true':     (T_OP,    TRUE),
    'false':    (T_OP,    FALSE),
    'r>':       (T_OP,    MRT),
    '>r':       (T_OP,    MTR),
    'r':        (T_OP,    R),
    'pc':       (T_OP,    PC),
    'rdrop':    (T_OP,    RDROP),
    'drop':     (T_OP,    DROP),
    'dup':      (T_OP,    DUP),
    'over':     (T_OP,    OVER),
    'swap':     (T_OP,    SWAP),
    '@':        (T_OP,    LB),
    '!':        (T_OP,    SB),
    'h@':       (T_OP,    LH),
    'h!':       (T_OP,    SH),
    'w@':       (T_OP,    LW),
    'w!':       (T_OP,    SW),
    'iod':      (T_OP,    IOD),
    'ior':      (T_OP,    IOR),
    'iodh':     (T_OP,    IODH),
    'iorh':     (T_OP,    IORH),
    'key':      (T_OP,    IOKEY),
    'emit':     (T_OP,    IOEMIT),
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
    '>x':       (T_OP,    MTX),
    'x':        (T_OP,    X),
    '>Y':       (T_OP,    MTY),
    'y':        (T_OP,    Y),
}
