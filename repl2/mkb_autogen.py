# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# THIS FILE IS AUTOMATICALLY GENERATED
# DO NOT MAKE EDITS HERE
# See codegen.py for details

# Markab VM opcode constants
NOP    =  0
RESET  =  1
JMP    =  2
JAL    =  3
RET    =  4
BZ     =  5
BFOR   =  6
U8     =  7
U16    =  8
I32    =  9
HALT   = 10
TRON   = 11
TROFF  = 12
IODUMP = 13
IOKEY  = 14
IORH   = 15
IOLOAD = 16
FOPEN  = 17
FREAD  = 18
FWRITE = 19
FSEEK  = 20
FTELL  = 21
FTRUNC = 22
FCLOSE = 23
MTR    = 24
R      = 25
CALL   = 26
PC     = 27
MTE    = 28
LB     = 29
SB     = 30
LH     = 31
SH     = 32
LW     = 33
SW     = 34
ADD    = 35
SUB    = 36
MUL    = 37
DIV    = 38
MOD    = 39
SLL    = 40
SRL    = 41
SRA    = 42
INV    = 43
XOR    = 44
OR     = 45
AND    = 46
GT     = 47
LT     = 48
EQ     = 49
NE     = 50
ZE     = 51
INC    = 52
DEC    = 53
IOEMIT = 54
IODOT  = 55
IODH   = 56
IOD    = 57
RDROP  = 58
DROP   = 59
DUP    = 60
OVER   = 61
SWAP   = 62
MTA    = 63
LBA    = 64
LBAI   = 65
AINC   = 66
ADEC   = 67
A      = 68
MTB    = 69
LBB    = 70
LBBI   = 71
SBBI   = 72
BINC   = 73
BDEC   = 74
B      = 75
TRUE   = 76
FALSE  = 77

# Markab VM opcode dictionary
OPCODES = {
    'NOP':     0,
    'RESET':   1,
    'JMP':     2,
    'JAL':     3,
    'RET':     4,
    'BZ':      5,
    'BFOR':    6,
    'U8':      7,
    'U16':     8,
    'I32':     9,
    'HALT':   10,
    'TRON':   11,
    'TROFF':  12,
    'IODUMP': 13,
    'IOKEY':  14,
    'IORH':   15,
    'IOLOAD': 16,
    'FOPEN':  17,
    'FREAD':  18,
    'FWRITE': 19,
    'FSEEK':  20,
    'FTELL':  21,
    'FTRUNC': 22,
    'FCLOSE': 23,
    'MTR':    24,
    'R':      25,
    'CALL':   26,
    'PC':     27,
    'MTE':    28,
    'LB':     29,
    'SB':     30,
    'LH':     31,
    'SH':     32,
    'LW':     33,
    'SW':     34,
    'ADD':    35,
    'SUB':    36,
    'MUL':    37,
    'DIV':    38,
    'MOD':    39,
    'SLL':    40,
    'SRL':    41,
    'SRA':    42,
    'INV':    43,
    'XOR':    44,
    'OR':     45,
    'AND':    46,
    'GT':     47,
    'LT':     48,
    'EQ':     49,
    'NE':     50,
    'ZE':     51,
    'INC':    52,
    'DEC':    53,
    'IOEMIT': 54,
    'IODOT':  55,
    'IODH':   56,
    'IOD':    57,
    'RDROP':  58,
    'DROP':   59,
    'DUP':    60,
    'OVER':   61,
    'SWAP':   62,
    'MTA':    63,
    'LBA':    64,
    'LBAI':   65,
    'AINC':   66,
    'ADEC':   67,
    'A':      68,
    'MTB':    69,
    'LBB':    70,
    'LBBI':   71,
    'SBBI':   72,
    'BINC':   73,
    'BDEC':   74,
    'B':      75,
    'TRUE':   76,
    'FALSE':  77,
}

# Markab VM memory map
Heap    = 0x0000
HeapRes = 0xE000
HeapMax = 0xE0FF
DP      = 0xE100
IN      = 0xE104
CORE_V  = 0xE108
EXT_V   = 0xE10C
MODE    = 0xE110
LASTCALL = 0xE118
NEST    = 0xE11C
BASE    = 0xE120
EOF     = 0xE124
LASTWORD = 0xE128
IRQRX   = 0xE12C
OK_EN   = 0xE130
LOADNEST = 0xE134
IRQERR  = 0xE138
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
ErrUnknown = 11
ErrNest = 12
ErrFilepath = 9
HashA   = 7
HashB   = 8
HashC   = 38335
HashBins = 64
HashMask = 63

# Markab language core vocabulary
CORE_VOC = {
    'Heap':     (T_CONST, 0x0000),
    'HeapRes':  (T_CONST, 0xE000),
    'HeapMax':  (T_CONST, 0xE0FF),
    'DP':       (T_CONST, 0xE100),
    'IN':       (T_CONST, 0xE104),
    'CORE_V':   (T_CONST, 0xE108),
    'EXT_V':    (T_CONST, 0xE10C),
    'MODE':     (T_CONST, 0xE110),
    'LASTCALL': (T_CONST, 0xE118),
    'NEST':     (T_CONST, 0xE11C),
    'BASE':     (T_CONST, 0xE120),
    'EOF':      (T_CONST, 0xE124),
    'LASTWORD': (T_CONST, 0xE128),
    'IRQRX':    (T_CONST, 0xE12C),
    'OK_EN':    (T_CONST, 0xE130),
    'LOADNEST': (T_CONST, 0xE134),
    'IRQERR':   (T_CONST, 0xE138),
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
    'ErrUnknown': (T_CONST, 11),
    'ErrNest':  (T_CONST, 12),
    'ErrFilepath': (T_CONST, 9),
    'HashA':    (T_CONST, 7),
    'HashB':    (T_CONST, 8),
    'HashC':    (T_CONST, 38335),
    'HashBins': (T_CONST, 64),
    'HashMask': (T_CONST, 63),
    'nop':      (T_OP,    NOP),
    'reset':    (T_OP,    RESET),
    'halt':     (T_OP,    HALT),
    'tron':     (T_OP,    TRON),
    'troff':    (T_OP,    TROFF),
    'dump':     (T_OP,    IODUMP),
    'key':      (T_OP,    IOKEY),
    'iorh':     (T_OP,    IORH),
    'load':     (T_OP,    IOLOAD),
    'fopen':    (T_OP,    FOPEN),
    'fread':    (T_OP,    FREAD),
    'fwrite':   (T_OP,    FWRITE),
    'fseek':    (T_OP,    FSEEK),
    'ftell':    (T_OP,    FTELL),
    'ftrunc':   (T_OP,    FTRUNC),
    'fclose':   (T_OP,    FCLOSE),
    '>r':       (T_OP,    MTR),
    'r':        (T_OP,    R),
    'call':     (T_OP,    CALL),
    'pc':       (T_OP,    PC),
    '>err':     (T_OP,    MTE),
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
