#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab bootstrap compiler
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
  IN, IBLen, IB, MemMax,

  CORE_VOC, T_VAR, T_CONST, T_OP, T_OBJ, T_IMM,
)
from markab_vm import VM

from os.path import basename, normpath


SRC_IN = 'kernel.mkb'
ROM_OUT = 'kernel.rom'

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

def parse_string(pos, src):
  """Parse string chars, returning (string, pos)"""
  s = ''
  count = len(src)
  for i in range(pos, count):
    if pos >= count:
      return (s, pos)
    c = src[pos]
    pos += 1
    if c == '"':       # skip all chars until '"'
      return (s, pos)
    s += c
  return (s, pos)

def skip_comment(pos, src):
  """Skip comment chars, returning (pos) for start of remaining text"""
  count = len(src)
  for i in range(pos, count):
    if pos >= count:
      return pos
    c = src[pos]
    pos += 1
    if c == ')':     # skip all chars until ')'
      return pos
  return pos

def next_word(pos, src):
  """Remove the next word from src, returning (word, pos)"""
  whitespace = [' ', '\t', '\r', '\n']
  word = ''
  count = len(src)
  for i in range(pos, count):
    if pos >= count:          # stop when src has no chars left
      return (word, pos)
    c = src[pos]              # get next char
    pos += 1
    if c in whitespace:
      if len(word) > 0:
        return (word, pos)    # stop for space after end of word
      else:
        continue              # skip space before start of next word
    word += c                 # append non-space chars to word
  return (word, pos)

def preprocess_mkb(src):
  """Preprocess Markab source code (comments, loads), return array of words"""
  obj = bytearray()
  pos = 0
  words = []
  count = len(src)
  for i in range(count):
    if pos >= count:
      break
    (word, pos) = next_word(pos, src)
    if word == '':
      continue
    if word == '(':
      pos = skip_comment(pos, src)
      continue
    if word == 'load"':
      # Parse string for a filename to load (CAUTION!)
      (filename, pos) = parse_string(pos, src)
      # Normalize filename and strip directory prefix to enforce that the load
      # file must be in the current working directory. This is a lazy way to
      # guard against security issues related to arbitrary filesystem access.
      norm_name = basename(normpath(filename))
      with open(filename) as f:
        words += preprocess_mkb(f.read())
    else:
      words.append(word)
  return words

def compile_mkb(src):
  """Compile Markab source code and return bytearray for a Markab VM rom"""
  obj = bytearray()
  words = preprocess_mkb(src)
  for i in range(len(words)):
    if words[i] == ':':
      if i>1 and words[i-1] == ':':
        pass
      else:
        print()
    if i>1 and words[i-2] in ['var', 'const', 'opcode']:
      if i>2 and words[i-3] == ':':
        pass
      else:
        print()
    print(words[i], end=' ')
  print()
  return obj

with open(ROM_OUT, 'w') as rom:
  with open(SRC_IN, 'r') as src:
    rom.buffer.write(compile_mkb(src.read()))
