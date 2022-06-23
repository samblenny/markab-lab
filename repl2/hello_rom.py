#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab hello world rom compiler
#
from mkb_autogen import (
  NOP, ADD, SUB, INC, DEC, MUL, AND, INV, OR, XOR, SLL, SRL, SRA,
  EQ, GT, LT, NE, ZE, TRUE, FALSE, JMP, JAL, CALL, RET,
  BZ, DRBLT, MTR, MRT, R, PC, RDROP, DROP, DUP, OVER, SWAP,
  U8, U16, I32, LB, SB, LH, SH, LW, SW, RESET,
  IOD, IOR, IODH, IORH, IOKEY, IOEMIT,
  MTA, LBA, LBAI,       AINC, ADEC, A,
  MTB, LBB, LBBI, SBBI, BINC, BDEC, B, MTX, X, MTY, Y,
  OPCODES,

  Boot, BootMax, Heap, HeapRes, HeapMax, DP,
  IN, IBLen, IB, PadLen, Pad, FmtLen, Fmt, MemMax,

  CORE_VOC, T_VAR, T_CONST, T_OP, T_OBJ, T_IMM,
)


ROM_FILE = 'hello.rom'

KERNEL_ASM = """
#          13  >a  a@+  1- for{  @a+   emit       }for
# addr:  0  1   2    3   4    5  *6*      7     8 9 10    11  12
        U8 13 MTA LBAI DEC  MTR LBAI IOEMIT DRBLT 6  0 RDROP RET
# addr: *13*
#            H   e   l   l   o  , <SP>  w   o   r   l   d  ! <LF> (14 bytes)
         14 72 101 108 108 111 44  32 119 111 114 108 100 33 10
"""

def filter(src):
  """Filter a comments and blank lines out of heredoc-style source string"""
  lines = src.strip().split("\n")
  lines = [L.split("#")[0].strip() for L in lines]    # filter comments
  lines = [L for L in lines if len(L) > 0]            # filter empty lines
  return lines

def get_opcode(word: str):
  """Attempt to resolve word to an instruction opcode constant"""
  if not word in CORE_VOC:
    return None
  (type_code, value) = CORE_VOC[word]
  if type_code != T_OP:
    return None
  return value

def compile_int(word: str):
  """Compile an integer literal"""
  n = int(word) & 0xffffffff
  if n <= 0xff:
    return bytearray([U8, n])
  elif n <= 0xffff:
    return bytearray([U16] + int.to_bytes(n, 2, 'little', signed=False))
  else:
    return bytearray([I32] + int.to_bytes(n, 4, 'little', signed=False))

def compile_kernel():
  """Compile a kernel image for the Markab VM"""
  obj = bytearray()
  for line in filter(KERNEL_ASM):
    for word in line.strip().split(" "):
      if word == '':
        continue
      if word in CORE_VOC:
        (type_code, value) = CORE_VOC[word]
        if type_code == T_OP:
          obj.extend(op)
          continue
        elif type_code == T_CONST:
          word = f"{value}"
      elif word in OPCODES:
        obj.extend(int.to_bytes(OPCODES[word], 1, 'little', signed=False))
        continue
      if word.isnumeric():
        obj.extend(int.to_bytes(int(word), 1, 'little', signed=False))
      else:
        raise Exception("not an op, not an int", word)
  return obj

with open(ROM_FILE, 'w') as f:
  f.buffer.write(compile_kernel())
