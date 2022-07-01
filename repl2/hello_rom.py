#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab hello world rom compiler
#
from mkb_autogen import OPCODES, CORE_VOC


ROM_FILE = 'hello.rom'

HELLO_ASM = """
#          12  >a  a@+  1- for{  @a+   emit      }for
# addr:  0  1   2    3   4    5  *6*      7    8 9 10  11
        U8 12 MTA LBAI DEC  MTR LBAI IOEMIT BFOR 6  0 RET
# addr: *12*
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

def compile_rom():
  """Compile a rom image for the Markab VM"""
  obj = bytearray()
  for line in filter(HELLO_ASM):
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
  f.buffer.write(compile_rom())
