#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# MarkabForth VM emulator
#
from ctypes import c_int32
from typing import Callable, Dict

from tokens import (
  NOP, ADD, SUB, MUL, AND, INV, OR, XOR, SLL, SRL, SRA, EQ, GT, LT, NE, ZE,
  JMP, JAL, RET, BZ, DRBLT, MTR, MRT, DROP, DUP, OVER, SWAP,
  U8, U16, I32, LB, SB, LH, SH, LW, SW, RESET, BREAK
)
from mem_map import IO, IOEnd, Boot, BootMax, MemMax

ROM_FILE = 'kernel.bin'
ERR_D_OVER = 1
ERR_D_UNDER = 2
ERR_BAD_ADDRESS = 3
ERR_BOOT_OVERFLOW = 4
ERR_BAD_INSTRUCTION = 5
ERR_R_OVER = 6
ERR_R_UNDER = 7

class VM:
  """
  VM emulates CPU, RAM, and peripheral state for a Markab virtual machine.
  """

  def __init__(self):
    """Initialize virtual CPU and RAM"""
    self.error = 0                  # Error code register
    self.base = 10                  # number Base for debug printing
    self.A = 0                      # Address/Accumulator register
    self.T = 0                      # Top of data stack
    self.S = 0                      # Second on data stack
    self.R = 0                      # top of Return stack
    self.PC = Boot                  # Program Counter
    self.Fence = 0                  # Fence between read-only and read/write
    self.DSDeep = 0                 # Data Stack Depth (count include T and S)
    self.RSDeep = 0                 # Return Stack Depth (count inlcudes R)
    self.DStack = [0] * 16          # Data Stack
    self.RStack = [0] * 16          # Return Stack
    self.ram = bytearray(MemMax+1)  # Random Access Memory
    #
    # Jump Table for instruction decoder
    self.jumpTable: Dict[int, Callable] = {}
    self.jumpTable[NOP  ] = self.nop
    self.jumpTable[ADD  ] = self.add
    self.jumpTable[SUB  ] = self.subtract
    self.jumpTable[MUL  ] = self.multiply
    self.jumpTable[AND  ] = self.and_
    self.jumpTable[INV  ] = self.invert
    self.jumpTable[OR   ] = self.or_
    self.jumpTable[XOR  ] = self.xor
    self.jumpTable[SLL  ] = self.shift_left_logical
    self.jumpTable[SRL  ] = self.shift_right_logical     # zero extend
    self.jumpTable[SRA  ] = self.shift_right_arithmetic  # sign extend
    self.jumpTable[EQ   ] = self.equal
    self.jumpTable[GT   ] = self.greater_than
    self.jumpTable[LT   ] = self.less_than
    self.jumpTable[NE   ] = self.not_equal
    self.jumpTable[ZE   ] = self.zero_equal
    self.jumpTable[JMP  ] = self.jump
    self.jumpTable[JAL  ] = self.jump_and_link           # call subroutine
    self.jumpTable[RET  ] = self.return_
    self.jumpTable[BZ   ] = self.branch_zero
    self.jumpTable[DRBLT] = self.dec_r_branch_less_than
    self.jumpTable[MTR  ] = self.move_t_to_r
    self.jumpTable[MRT  ] = self.move_r_to_t
    self.jumpTable[RESET] = self.reset
    self.jumpTable[BREAK] = self.break_
    self.jumpTable[DROP ] = self.drop
    self.jumpTable[DUP  ] = self.dup
    self.jumpTable[OVER ] = self.over
    self.jumpTable[SWAP ] = self.swap
    self.jumpTable[U8   ] = self.u8_literal
    self.jumpTable[U16  ] = self.u16_literal
    self.jumpTable[I32  ] = self.i32_literal
    self.jumpTable[LB   ] = self.load_byte
    self.jumpTable[SB   ] = self.store_byte
    self.jumpTable[LH   ] = self.load_halfword
    self.jumpTable[SH   ] = self.store_halfword
    self.jumpTable[LW   ] = self.load_word
    self.jumpTable[SW   ] = self.store_word

  def _set_pc(self, addr):
    """Set Program Counter with range check"""
    if (addr > MemMax) or (addr > 0xffff):
      self.error = ERR_BAD_ADDRESS
      return
    self.PC = addr

  def _next_instruction(self):
    """Load an instruction from location pointed to by Program Counter (PC)"""
    pc = self.PC
    self._set_pc(pc+1)
    return self.ram[pc]

  def _load_boot(self, code):
    """Load byte array of machine code instructions into the Boot memory"""
    n = len(code)
    if n > (BootMax+1)-Boot:
      self.error = ERR_BOOT_OVERFLOW
      return
    self.ram[Boot:Boot+n] = code[0:]

  def _warm_boot(self, code, max_cycles=1):
    """Load a byte array of machine code into Boot memory, then run it."""
    self._load_boot(code)
    self._set_pc(Boot)
    self._step(max_cycles)

  def _step(self, max_cycles):
    """Step the virtual CPU clock to run up to max_cycles instructions"""
    for _ in range(max_cycles):
      t = self._next_instruction()
      if self.RSDeep == 0 and t == RET:
        return
      if t in self.jumpTable:
        (self.jumpTable[t])()
      else:
        self.error = ERR_BAD_INSTRUCTION
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

  def load_byte(self):
    """Load a uint8 (1 byte) from memory address T, saving result in T"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax):
      self.error = ERR_BAD_ADDRESS
      return
    self.T = int.from_bytes(self.ram[addr:addr+1], 'little', signed=False)

  def store_byte(self):
    """Store low byte of S (uint8) at memory address T"""
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

  def jump_and_link(self):
    """Jump to subroutine after pushing old value of PC to return stack"""
    if self.RSDeep > 16:
      self.reset()
      self.error = ERR_R_OVER
      return
    # read a 16-bit address from the instruction stream
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+2], 'little', signed=False)
    self._set_pc(pc+2)
    # push the current Program Counter (PC) to return stack
    if self.RSDeep > 0:
      rSecond = self.RSDeep - 1
      self.RStack[rSecond] = self.PC
    self.R = self.PC
    self.RSDeep += 1
    # set Program Counter to the new address
    self.PC = n

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

  def load_word(self):
    """Load a signed int32 (word = 4 bytes) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-3):
      self.error = ERR_BAD_ADDRESS
      return
    self.T = int.from_bytes(self.ram[addr:addr+4], 'little', signed=True)

  def greater_than(self):
    """Evaluate S > T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s > t else 0)

  def invert(self):
    """Invert the bits of T (ones' complement negation)"""
    self._op_t(lambda t: ~ t)

  def jump(self):
    """Jump to subroutine at address read from instruction stream"""
    # read a 16-bit address from the instruction stream
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+2], 'little', signed=False)
    self._set_pc(pc+2)
    # set program counter to the new address
    self.PC = n

  def branch_zero(self):
    """Branch to address read from instruction stream if T == 0"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    pc = self.PC
    if self.T == 0:
      # Branch past conditional block: Set PC to address literal
      n = int.from_bytes(self.ram[pc:pc+2], 'little', signed=False)
      self.PC = n
    else:
      # Enter conditional block: Advance PC past address literal
      self._set_pc(pc+2)

  def dec_r_branch_less_than(self):
    """Decrement R and Branch to address if R is Less Than 0"""
    if self.RSDeep < 1:
      self.error = ERR_R_UNDER
      return
    self.R -= 1
    pc = self.PC
    if self.R >= 0:
      # Keep looping: Set PC to address literal
      n = int.from_bytes(self.ram[pc:pc+2], 'little', signed=False)
      self.PC = n
    else:
      # End of loop: Advance PC past address literal
      self._set_pc(pc+2)

  def less_than(self):
    """Evaluate S < T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s < t else 0)

  def u16_literal(self):
    """Read uint16 halfword (2 bytes) literal, zero-extend it, push as T"""
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+2], 'little', signed=False)
    self._set_pc(pc+2)
    self._push(n)

  def i32_literal(self):
    """Read int32 word (4 bytes) signed literal, push as T"""
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+4], 'little', signed=True)
    self._set_pc(pc+4)
    self._push(n)

  def u8_literal(self):
    """Read uint8 byte literal, zero-extend it, push as T"""
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+1], 'little', signed=False)
    self._set_pc(pc+1)
    self._push(n)

  def subtract(self):
    """Subtract T from S, store result in S, drop T"""
    self._op_st(lambda s, t: s - t)

  def multiply(self):
    """Multiply S by T, store result in S, drop T"""
    self._op_st(lambda s, t: s * t)

  def not_equal(self):
    """Evaluate S <> T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s != t else 0)

  def or_(self):
    """Store bitwise OR of S with T into S, then drop T"""
    self._op_st(lambda s, t: s | t)
    pass

  def over(self):
    """Push a copy of S"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    self._push(self.S)

  def add(self):
    """Add T to S, store result in S, drop T"""
    self._op_st(lambda s, t: s + t)

  def reset(self):
    """Reset the data stack, return stack and error code"""
    self.DSDeep = 0
    self.RSDeep = 0
    self.error = 0

  def return_(self):
    """Return from subroutine, taking address from return stack"""
    if self.RSDeep < 1:
      self.reset()
      self.error = ERR_R_UNDER
      return
    # Set program counter from top of return stack
    self.PC = self.R
    # Drop top of return stack
    if self.RSDeep > 1:
      rSecond = self.RSDeep - 2
      self.R = self.RStack[rSecond]
    self.RSDeep -= 1

  def break_(self):
    """Breakpoint: stop normal Program Counter and enter debug mode"""
    # TODO: figure out how to handle BREAK
    pass

  def move_r_to_t(self):
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

  def shift_left_logical(self):
    """Shift S left by T, store result in S, drop T"""
    self._op_st(lambda s, t: s << t)

  def shift_right_arithmetic(self):
    """Signed (arithmetic) shift S right by T, store result in S, drop T"""
    # Python right shift is always an arithmetic (signed) shift
    self._op_st(lambda s, t: s >> t)

  def shift_right_logical(self):
    """Unsigned (logic) shift S right by T, store result in S, drop T"""
    # Mask first because Python right shift is always signed
    self._op_st(lambda s, t: (s & 0xffffffff) >> t)

  def store_word(self):
    """Store word (4 bytes) from S as signed int32 at memory address T"""
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
    """Swap S with T"""
    if self.DSDeep < 2:
      self.error = ERR_D_UNDER
      return
    tmp = self.T
    self.T = self.S
    self.S = tmp

  def move_t_to_r(self):
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

  def load_halfword(self):
    """Load halfword (2 bytes, zero-extended) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error = ERR_D_UNDER
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-1):
      self.error = ERR_BAD_ADDRESS
      return
    self.T = int.from_bytes(self.ram[addr:addr+2], 'little', signed=False)

  def store_halfword(self):
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

  def zero_equal(self):
    """Evaluate 0 == T (true:-1, false:0), store result in T"""
    self._op_t(lambda t: -1 if 0 == t else 0)

  def _hex(self):
    """Set debug printing number base to 16"""
    self.base = 16

  def _decimal(self):
    """Set debug printing number base to 10"""
    self.base = 10

  def _log_ds(self):
    """Log (debug print) the data stack in the manner of .S"""
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
      self._clear_error()
    else:
      print("  OK")

  def _log_rs(self):
    """Log (debug print) the return stack in the manner of .S"""
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
      self._clear_error()
    else:
      print("  OK")

  def _clear_error(self):
    """Clear VM error status code"""
    self.error = 0
