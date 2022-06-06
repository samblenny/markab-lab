#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# MarkabForth VM emulator
#
from tokens  import get_token, get_opcode
from mem_map import (
  IO, IOEnd, A, T, R, DSDeep, RSDeep, DStack, RStack, IP, Fence, MemMax)

ROM_FILE = 'kernel.bin'
ERR_D_OVER = 1
ERR_D_UNDER = 2

class VMTask:
  """
  VMTask manages CPU, RAM, and peripheral state for one markabForth task.
  """
  ram = bytearray(MemMax)
  error = 0

  def push(self, n):
    deep = self.ram[DSDeep]
    if deep > 16:
      self.error = ERR_D_OVER
      return
    second = DStack + (4 * (deep-1))
    self.ram[second:second+4] = self.ram[T:T+4]
    self.ram[T:T+4] = int.to_bytes((n & 0xffffffff), 4, 'little')
    self.ram[DSDeep] = deep+1

  def drop(self):
    deep = self.ram[DSDeep]
    if deep < 1:
      self.error = ERR_D_UNDER
      return
    second = DStack + (4 * (deep-1))
    self.ram[T:T+4] = self.ram[second:second+4]
    self.ram[DSDeep] = deep-1

  def dotS(self):
    if self.error != 0:
      print(f"  ERR: {self.error}")
      self.clearError()
      return
    deep = self.ram[DSDeep]
    if deep > 1:
      for i in range(deep-1):
        x = DStack + (4 * i)
        n = int.from_bytes(self.ram[x:x+4], 'little')
        print(f" {n}", end='')
    if deep > 0:
      n = int.from_bytes(self.ram[T:T+4], 'little')
      print(f" {n}  OK")
    else:
      print(" Stack is empty  OK")

  def clearError(self):
    self.error = 0
