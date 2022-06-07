#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# MarkabForth VM emulator
#
from ctypes import c_uint32, c_int32

from tokens  import get_token, get_opcode
from mem_map import IO, IOEnd, Boot, BootMax, MemMax

ROM_FILE = 'kernel.bin'
ERR_D_OVER = 1
ERR_D_UNDER = 2
ERR_ADDR_OOR = 3
ERR_BOOT_OVERFLOW = 4
ERR_BAD_TOKEN = 5

class VMTask:
  """
  VMTask manages CPU, RAM, and peripheral state for one markabForth task.
  """

  def __init__(self):
    """Initialize each instance of VMTask with its own state variables"""
    self.error = 0                  # Error code register
    self.base = 10                  # number Base for debug printing
    self.A = 0                      # Address/Accumulator register
    self.T = 0                      # Top of data stack
    self.S = 0                      # Second on data stack
    self.R = 0                      # top of Return stack
    self.IP = Boot                  # Instruction Pointer
    self.Fence = 0                  # Fence between read-only and read/write
    self.DSDeep = 0                 # Data Stack Depth (count include T and S)
    self.RSDeep = 0                 # Return Stack Depth (count inlcudes R)
    self.DStack = [0] * 16          # Data Stack
    self.RStack = [0] * 16          # Return Stack
    self.ram = bytearray(MemMax+1)  # Random Access Memory

  def _setIP(self, addr):
    """Set Instruction Pointer with range check"""
    if (addr > MemMax) or (addr > 0xffff):
      self.error = ERR_ADDR_OOR
      return
    self.IP = addr

  def _nextToken(self):
    """Get the next token from ram[IP]"""
    _ip = self.IP
    self._setIP(_ip+1)
    return self.ram[_ip]

  def _loadBoot(self, code):
    """Load bytearray of token code into the Boot..BootMax memory region"""
    n = len(code)
    if n > (BootMax+1)-Boot:
      self.error = ERR_BOOT_OVERFLOW
      return
    self.ram[Boot:Boot+n] = code[0:]

  def _warmBoot(self, code):
    """Load a bytearray of token code into Boot..BootMax, then run it."""
    self._loadBoot(code)
    self._setIP(Boot)
    self._step(len(code))

  def _step(self, count):
    """Step the virtual CPU for enough cycles to consume count tokens"""
    stopIP = self.IP + count
    for _ in range(count):
      t = self._nextToken()
      if t == get_token('Lit8'):
        self.lit8()
      elif t == get_token('Lit16'):
        self.lit16()
      elif t == get_token('Lit32'):
        self.lit32()
      elif t == get_token('Return'):
        return
      else:
        self.error = ERR_BAD_TOKEN
        return

  def _op_st(self, fn):
    """Apply operation λ(S,T), storing the result in S and dropping T"""
    if self.DSDeep < 2:
      self.error = ERR_D_UNDER
      return
    self.S = c_int32(fn(self.S, self.T)).value
    self.drop()

  def _op_t(self, fn):
    """Apply operation λ(T), storing the result in T"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    self.T = c_int32(fn(self.T)).value

  def _push(self, n):
    """Push n onto the data stack as a 32-bit signed integer"""
    deep = self.DSDeep
    if deep > 17:
      self.error = ERR_D_OVER
      return
    if deep > 1:
      third = deep-2
      self.DStack[third] = self.S
    self.S = self.T
    self.T = c_int32(n).value
    self.DSDeep += 1

  def nop(self):
    """Do nothing, but consume a little time for the non-doing"""
    pass

  def and_(self):
    """Store bitwise AND of S with T into S, then drop T"""
    self._op_st(lambda s, t: s & t)

  def bFetch(self):
    pass

  def bStore(self):
    pass

  def call(self):
    pass

  def drop(self):
    """Drop T, the top item of the data stack"""
    deep = self.DSDeep
    if deep < 1:
      self.error = ERR_D_UNDER
      return
    self.T = self.S
    if deep > 2:
      third = deep-3
      self.S = self.DStack[third]
    self.DSDeep -= 1

  def dup(self):
    """Push a copy of T"""
    self._push(self.T)

  def equal(self):
    pass

  def fetch(self):
    pass

  def greater(self):
    pass

  def invert(self):
    """Invert the bits of T (ones' complement negation)"""
    self._op_t(lambda t: ~ t)

  def jump(self):
    pass

  def less(self):
    pass

  def lit16(self):
    """Read uint16 (2 bytes) from token stream, zero-extend it, push as T"""
    ip = self.IP
    n = int.from_bytes(self.ram[ip:ip+2], 'little', signed=False)
    self._setIP(ip+2)
    self._push(n)

  def lit32(self):
    """Read int32 (4 bytes) from token stream, push as T"""
    ip = self.IP
    n = int.from_bytes(self.ram[ip:ip+4], 'little', signed=True)
    self._setIP(ip+4)
    self._push(n)

  def lit8(self):
    """Read uint8 (1 byte) from token stream, zero-extend it, push as T"""
    ip = self.IP
    n = int.from_bytes(self.ram[ip:ip+1], 'little', signed=False)
    self._setIP(ip+1)
    self._push(n)

  def minus(self):
    """Subtract T from S, store result in S, drop T"""
    self._op_st(lambda s, t: s - t)

  def mul(self):
    """Multiply S by T, store result in S, drop T"""
    self._op_st(lambda s, t: s * t)

  def notEq(self):
    pass

  def or_(self):
    """Store bitwise OR of S with T into S, then drop T"""
    self._op_st(lambda s, t: s | t)
    pass

  def over(self):
    pass

  def plus(self):
    """Add T to S, store result in S, drop T"""
    self._op_st(lambda s, t: s + t)

  def reset(self):
    """Reset the data stack, return stack and error code"""
    self.DSDeep = 0
    self.RSDeep = 0
    self.error = 0

  def return_(self):
    pass

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
    """Store bitwise XOR of S with T into S, then drop T"""
    self._op_st(lambda s, t: s ^ t)
    pass

  def zeroEq(self):
    pass

  def _hex(self):
    """Set debug printing number base to 16"""
    self.base = 16

  def _decimal(self):
    """Set debug printing number base to 10"""
    self.base = 10

  def _dotS(self):
    """Print the data stack in the manner of .S"""
    if self.error != 0:
      print(f"  ERR: {self.error}")
      self._clearError()
      return
    print(" ", end='')
    deep = self.DSDeep
    if deep > 2:
      for i in range(deep-2):
        n = self.DStack[i]
        if self.base == 16:
          print(f" {n&0xffffffff:x}", end='')
        else:
          print(f" {n}", end='')
    if deep > 1:
      if self.base == 16:
        print(f" {self.S&0xffffffff:x}", end='')
      else:
        print(f" {self.S}", end='')
    if deep > 0:
      if self.base == 16:
        print(f" {self.T&0xffffffff:x}  OK")
      else:
        print(f" {self.T}  OK")
    else:
      print(" Stack is empty  OK")

  def _clearError(self):
    """Clear VM error status code"""
    self.error = 0

  def _load(self, tokens):
    """Copy a bytearray of token code to RAM starting at reset vector"""
