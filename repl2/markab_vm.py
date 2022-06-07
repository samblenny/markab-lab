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
ERR_BAD_ADDRESS = 3
ERR_BOOT_OVERFLOW = 4
ERR_BAD_TOKEN = 5
ERR_R_OVER = 6
ERR_R_UNDER = 7

class VM:
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
      self.error = ERR_BAD_ADDRESS
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
      self.reset()             # Clear data and return stacks
      self.error = ERR_D_OVER  # Set error code
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
    """Fetch an unsigned uint8 (1 byte) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax):
      self.error = ERR_BAD_ADDRESS
      return
    self.T = int.from_bytes(self.ram[addr:addr+1], 'little', signed=False)

  def bStore(self):
    """Store low bytes from S (uint8) at memory address T"""
    if self.DSDeep < 2:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax) or (addr <= self.Fence):
      self.error = ERR_BAD_ADDRESS
      return
    x = int.to_bytes((self.S & 0xff), 1, 'little', signed=False)
    self.ram[addr:addr+1] = x
    self.drop()
    self.drop()

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
    """Evaluate S == T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s == t else 0)

  def fetch(self):
    """Fetch a signed uint32 (4 bytes) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-3):
      self.error = ERR_BAD_ADDRESS
      return
    self.T = int.from_bytes(self.ram[addr:addr+4], 'little', signed=True)

  def greater(self):
    """Evaluate S > T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s > t else 0)

  def invert(self):
    """Invert the bits of T (ones' complement negation)"""
    self._op_t(lambda t: ~ t)

  def jump(self):
    pass

  def less(self):
    """Evaluate S < T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s < t else 0)

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
    """Evaluate S <> T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s != t else 0)

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
    """Move top of return stack (R) to top of data stack (T)"""
    if self.RSDeep < 1:
      self.reset()
      self.error = ERR_R_UNDER
      return
    if self.DSDeep > 17:
      self.reset()
      self.error = ERR_D_OVER
      return
    self._push(self.R)
    if self.RSDeep > 1:
      rSecond = self.RSDeep - 2
      self.R = self.RStack[rSecond]
    self.RSDeep -= 1

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
    """Store all 4 bytes of S (signed int32) at memory address T"""
    if self.DSDeep < 2:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-3) or (addr <= self.Fence):
      self.error = ERR_BAD_ADDRESS
      return
    x = int.to_bytes(c_int32(self.S).value, 4, 'little', signed=True)
    self.ram[addr:addr+4] = x
    self.drop()
    self.drop()

  def swap(self):
    pass

  def toR(self):
    """Move top of data stack (T) to top of return stack (R)"""
    if self.RSDeep > 16:
      self.reset()
      self.error = ERR_R_OVER
      return
    if self.DSDeep < 1:
      self.reset()
      self.error = ERR_D_UNDER
      return
    if self.RSDeep > 0:
      rSecond = self.RSDeep - 1
      self.RStack[rSecond] = self.R
    self.R = self.T
    self.RSDeep += 1
    self.drop()

  def wFetch(self):
    """Fetch an unsigned uint16 (2 bytes) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-1):
      self.error = ERR_BAD_ADDRESS
      return
    self.T = int.from_bytes(self.ram[addr:addr+2], 'little', signed=False)

  def wStore(self):
    """Store low 2 bytes from S (uint16) at memory address T"""
    if self.DSDeep < 2:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-1) or (addr <= self.Fence):
      self.error = ERR_BAD_ADDRESS
      return
    x = int.to_bytes((self.S & 0xffff), 2, 'little', signed=False)
    self.ram[addr:addr+2] = x
    self.drop()
    self.drop()

  def xor(self):
    """Store bitwise XOR of S with T into S, then drop T"""
    self._op_st(lambda s, t: s ^ t)

  def zeroEq(self):
    """Evaluate 0 == T (true:-1, false:0), store result in T"""
    self._op_t(lambda t: -1 if 0 == t else 0)

  def _hex(self):
    """Set debug printing number base to 16"""
    self.base = 16

  def _decimal(self):
    """Set debug printing number base to 10"""
    self.base = 10

  def _dotS(self):
    """Print the data stack in the manner of .S"""
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
        print(f" {self.T&0xffffffff:x}", end='')
      else:
        print(f" {self.T}", end='')
    else:
      print(" Stack is empty", end='')
    if self.error != 0:
      print(f"  ERR{self.error}")
      self._clearError()
    else:
      print("  OK")

  def _dotRet(self):
    """Print the return stack in the manner of .S"""
    print(" ", end='')
    deep = self.RSDeep
    if deep > 1:
      for i in range(deep-1):
        n = self.RStack[i]
        if self.base == 16:
          print(f" {n&0xffffffff:x}", end='')
        else:
          print(f" {n}", end='')
    if deep > 0:
      if self.base == 16:
        print(f" {self.R&0xffffffff:x}", end='')
      else:
        print(f" {self.R}", end='')
    else:
      print(" R-Stack is empty", end='')
    if self.error != 0:
      print(f"  ERR{self.error}")
      self._clearError()
    else:
      print("  OK")

  def _clearError(self):
    """Clear VM error status code"""
    self.error = 0

  def _load(self, tokens):
    """Copy a bytearray of token code to RAM starting at reset vector"""
