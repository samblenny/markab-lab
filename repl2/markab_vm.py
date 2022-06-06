#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# MarkabForth VM emulator
#
from ctypes import c_uint32, c_int32

from tokens  import get_token, get_opcode
from mem_map import (
  IO, IOEnd, A, T, S, R, DSDeep, RSDeep, DStack, RStack, IP, Fence, MemMax)

ROM_FILE = 'kernel.bin'
ERR_D_OVER = 1
ERR_D_UNDER = 2

class VMTask:
  """
  VMTask manages CPU, RAM, and peripheral state for one markabForth task.
  """

  def __init__(self):
    """Initialize each instance of VMTask with its own state variables"""
    self.ram = bytearray(MemMax+1)
    self.error = 0

  def _op_st(self, fn):
    """Apply operation λ(S,T), storing the result in S and dropping T"""
    deep = self.ram[DSDeep]
    if deep < 2:
      self.error = ERR_D_UNDER
      return
    _t = int.from_bytes(self.ram[T:T+4], 'little', signed=True)
    _s = int.from_bytes(self.ram[S:S+4], 'little', signed=True)
    _s = fn(_s, _t) & 0xffffffff
    self.ram[S:S+4] = int.to_bytes(_s, 4, 'little')
    self.drop()

  def nop(self):
    pass

  def and_(self):
    pass

  def bFetch(self):
    pass

  def bStore(self):
    pass

  def drop(self):
    """Drop T, the top item of the data stack"""
    deep = self.ram[DSDeep]
    if deep < 1:
      self.error = ERR_D_UNDER
      return
    self.ram[T:T+4] = self.ram[S:S+4]
    if deep > 2:
      third = DStack + (4 * (deep-3))
      self.ram[S:S+4] = self.ram[third:third+4]
    self.ram[DSDeep] = deep-1

  def dup(self):
    """Push a copy of T"""
    _t = int.from_bytes(self.ram[T:T+4], 'little', signed=True)
    self.push(_t)

  def equal(self):
    pass

  def fetch(self):
    pass

  def greater(self):
    pass

  def invert(self):
    pass

  def less(self):
    pass

  def minus(self):
    """Subtract T from S, store result in S, drop T"""
    self._op_st(lambda s, t: s - t)

  def mul(self):
    """Multiply S by T, store result in S, drop T"""
    self._op_st(lambda s, t: s * t)

  def notEq(self):
    pass

  def or_(self):
    pass

  def over(self):
    pass

  def plus(self):
    """Add T to S, store result in S, drop T"""
    self._op_st(lambda s, t: s + t)

  def push(self, n):
    """Push n onto the data stack as a 32-bit signed integer"""
    deep = self.ram[DSDeep]
    if deep > 17:
      self.error = ERR_D_OVER
      return
    if deep > 1:
      third = DStack + (4 * (deep-2))
      self.ram[third:third+4] = self.ram[S:S+4]
    self.ram[S:S+4] = self.ram[T:T+4]
    n = c_int32(n).value
    self.ram[T:T+4] = int.to_bytes(n, 4, 'little', signed=True)
    self.ram[DSDeep] = deep+1

  def reset(self):
    """Reset the data stack, return stack and error code"""
    self.ram[DSDeep] = b'\x00'
    self.ram[RSDeep] = b'\x00'
    self.error = 0

  def rFrom(self):
    pass

  def shiftLeft(self):
    """Shift S left by T, store result in S, drop T"""
    self._op_st(lambda s, t: s << t)

  def shiftRightI32(self):
    """Signed (arithmetic) shift S right by T, store result in S, drop T"""
    # Python right shift is always an arithmetic (signed) shift
    self._op_st(lambda s, t: s >> t)

  def shiftRightU32(self):
    """Unsigned (logic) shift S right by T, store result in S, drop T"""
    # Mask first because Python right shift is always signed
    self._op_st(lambda s, t: (s & 0xffffffff) >> t)

  def store(self):
    pass

  def swap(self):
    pass

  def toR(self):
    pass

  def wFetch(self):
    pass

  def wStore(self):
    pass

  def xor(self):
    pass

  def zeroEq(self):
    pass

  def dotS(self, hex=False):
    """Print the data stack in the manner of .S"""
    if self.error != 0:
      print(f"  ERR: {self.error}")
      self.clearError()
      return
    print(" ", end='')
    deep = self.ram[DSDeep]
    if deep > 2:
      for i in range(deep-2):
        x = DStack + (4 * i)
        n = int.from_bytes(self.ram[x:x+4], 'little', signed=True)
        if hex:
          print(f" {n&0xffffffff:x}", end='')
        else:
          print(f" {n}", end='')
    if deep > 1:
      _s = int.from_bytes(self.ram[S:S+4], 'little', signed=True)
      if hex:
        print(f" {_s&0xffffffff:x}", end='')
      else:
        print(f" {_s}", end='')
    if deep > 0:
      _t = int.from_bytes(self.ram[T:T+4], 'little', signed=True)
      if hex:
        print(f" {_t&0xffffffff:x}  OK")
      else:
        print(f" {_t}  OK")
    else:
      print(" Stack is empty  OK")

  def clearError(self):
    self.error = 0
