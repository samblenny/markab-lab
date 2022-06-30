#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab VM emulator
#
import cProfile
from ctypes import c_int32
from typing import Callable, Dict
import readline
import sys
import os

from mkb_autogen import (
  NOP, ADD, SUB, INC, DEC, MUL, AND, INV, OR, XOR, SLL, SRL, SRA,
  EQ, GT, LT, NE, ZE, TRUE, FALSE, JMP, JAL, CALL, RET,
  BZ, BFOR, MTR, MRT, RDROP, R, PC, ERR, DROP, DUP, OVER, SWAP,
  U8, U16, I32, LB, SB, LH, SH, LW, SW, RESET, FENCE, CLERR,
  IOD, IOR, IODH, IORH, IOKEY, IOEMIT, IODOT, IODUMP, TRON, TROFF,
  MTA, LBA, LBAI,       AINC, ADEC, A,
  MTB, LBB, LBBI, SBBI, BINC, BDEC, B, MTX, X, MTY, Y,

  Heap, HeapRes, HeapMax, MemMax,
  OPCODES,
)


# This controls debug tracing and dissasembly. DEBUG is a global enable for
# tracing, but it won't do anything unless the code uses a TRON instruction to
# turn tracing on (TROFF turns it back off). Intended usage is that you bracket
# a specific area of code to be traced with TRON/TROFF. Otherwise, the huge
# amount of trace data from dictionary lookups will be too noisy to follow.
#
DEBUG = False #True

# Set this to True if you want a profile report from cProfile
#  (see https://docs.python.org/3/library/profile.html)
PROFILE = False #True

ROM_FILE = 'kernel.rom'
ERR_D_OVER = 1
ERR_D_UNDER = 2
ERR_BAD_ADDRESS = 3
ERR_BOOT_OVERFLOW = 4
ERR_BAD_INSTRUCTION = 5
ERR_R_OVER = 6
ERR_R_UNDER = 7
ERR_MAX_CYCLES = 8

# Configure STDIN/STDOUT at load-time for use utf-8 encoding.
# For documentation on arguments to `reconfigure()`, see
# https://docs.python.org/3/library/io.html#io.TextIOWrapper
sys.stdout.reconfigure(encoding='utf-8', line_buffering=True)
sys.stdin.reconfigure(encoding='utf-8', line_buffering=True)

# Turn off readline's automatic appending of input to the history file
readline.write_history_file = lambda *args: None

class VM:
  """
  Emulate CPU, RAM, and peripherals for a Markab virtual machine instance.
  """

  def __init__(self, echo=False):
    """Initialize virtual CPU, RAM, and peripherals"""
    self.ERR = 0                    # Error code register
    self.base = 10                  # number Base for debug printing
    self.A = 0                      # register for source address or scratch
    self.B = 0                      # register for destination addr or scratch
    self.X = 0                      # scratch (temporary) register
    self.Y = 0                      # scratch (temporary) register
    self.T = 0                      # Top of data stack
    self.S = 0                      # Second on data stack
    self.R = 0                      # top of Return stack
    self.PC = Heap                  # Program Counter
    self.Fence = 0                  # Fence between read-only and read/write
    self.DSDeep = 0                 # Data Stack Depth (count include T and S)
    self.RSDeep = 0                 # Return Stack Depth (count inlcudes R)
    self.DStack = [0] * 16          # Data Stack
    self.RStack = [0] * 16          # Return Stack
    self.ram = bytearray(MemMax+1)  # Random Access Memory
    self.inbuf = ''                 # Input buffer
    self.echo = echo                # Echo depends on tty vs pip, etc.
    self.cycle_count = 65535        # Counter to break infinite loops
    # Debug tracing on/off
    self.dbg_trace_enable = False   # Debug trace is noisy, so default=off
    # Debug symbols
    self.dbg_addrs = []
    self.dbg_names = []
    #
    # Jump Table for instruction decoder
    self.jumpTable: Dict[int, Callable] = {}
    self.jumpTable[NOP  ] = self.nop
    self.jumpTable[ADD  ] = self.add
    self.jumpTable[SUB  ] = self.subtract
    self.jumpTable[INC  ] = self.increment
    self.jumpTable[DEC  ] = self.decrement
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
    self.jumpTable[TRUE ] = self.true_
    self.jumpTable[FALSE] = self.false_
    self.jumpTable[JMP  ] = self.jump
    self.jumpTable[JAL  ] = self.jump_and_link
    self.jumpTable[CALL ] = self.call
    self.jumpTable[RET  ] = self.return_
    self.jumpTable[BZ   ] = self.branch_zero
    self.jumpTable[BFOR ] = self.branch_for_loop
    self.jumpTable[MTR  ] = self.move_t_to_r
    self.jumpTable[MRT  ] = self.move_r_to_t
    self.jumpTable[RDROP] = self.r_drop
    self.jumpTable[R    ] = self.r_
    self.jumpTable[PC   ] = self.pc_
    self.jumpTable[ERR  ] = self.err_
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
    self.jumpTable[RESET] = self.reset
    self.jumpTable[FENCE] = self.set_fence
    self.jumpTable[CLERR] = self.clear_error
    self.jumpTable[IOD  ] = self.io_data_stack
    self.jumpTable[IOR  ] = self.io_return_stack
    self.jumpTable[IODH ] = self.io_data_stack_hex
    self.jumpTable[IORH ] = self.io_return_stack_hex
    self.jumpTable[IOKEY] = self.io_key
    self.jumpTable[IOEMIT] = self.io_emit
    self.jumpTable[IODOT] = self.io_dot
    self.jumpTable[IODUMP] = self.io_dump
    self.jumpTable[TRON ] = self.trace_on
    self.jumpTable[TROFF] = self.trace_off
    self.jumpTable[MTA  ] = self.move_t_to_a
    self.jumpTable[LBA  ] = self.load_byte_a
    self.jumpTable[LBAI ] = self.load_byte_a_increment
    self.jumpTable[AINC ] = self.a_increment
    self.jumpTable[ADEC ] = self.a_decrement
    self.jumpTable[A    ] = self.a_
    self.jumpTable[MTB  ] = self.move_t_to_b
    self.jumpTable[LBB  ] = self.load_byte_b
    self.jumpTable[LBBI ] = self.load_byte_b_increment
    self.jumpTable[SBBI ] = self.store_byte_b_increment
    self.jumpTable[BINC ] = self.b_increment
    self.jumpTable[BDEC ] = self.b_decrement
    self.jumpTable[B    ] = self.b_
    self.jumpTable[MTX  ] = self.move_t_to_x
    self.jumpTable[X    ] = self.x_
    self.jumpTable[MTY  ] = self.move_t_to_y
    self.jumpTable[Y    ] = self.y_

  def dbg_add_symbol(self, addr, name):
    """Add a debug symbol entry to the symbol table"""
    self.dbg_addrs.append(int(addr))
    self.dbg_names.append(name)

  def dbg_addr_for(self, addr):
    """Given a name, return the address of the symbol it belongs to"""
    for (i, s_name) in enumerate(self.dbg_names):
      if name == s_name:
        return self.dbg_addr[i]
    return 0

  def dbg_name_for(self, addr):
    """Given an address, return the name of the symbol it belongs to"""
    for (i, s_addr) in enumerate(self.dbg_addrs):
      if addr >= s_addr:
        return self.dbg_names[i]
    return '<???>'

  def error(self, code):
    """Set the ERR register with an error code"""
    if DEBUG:
      print(f"<<ERR{code}>>", end='')
    self.ERR = code

  def _load_rom(self, code):
    """Load byte array of rom image into memory"""
    n = len(code)
    if n > (HeapRes-Heap):
      self.error(ERR_BOOT_OVERFLOW)
      return
    self.ram[Heap:Heap+n] = code[0:]

  def _warm_boot(self, code, max_cycles=1):
    """Load a byte array of machine code into memory, then run it."""
    self.ERR = 0
    self._load_rom(code)
    self.PC = Heap          # hardcode boot vector to start of heap (for now)
    self._step(max_cycles)

  def _step(self, max_cycles):
    """Step the virtual CPU clock to run up to max_cycles instructions"""
    self.cycle_count = max_cycles
    while self.cycle_count > 0:
      self.cycle_count -= 1
      pc = self.PC
      self.PC += 1
      op = self.ram[pc]
      if DEBUG and self.dbg_trace_enable:
        name = [k for (k,v) in OPCODES.items() if v == op]
        if len(name) == 1:
          name = name[0]
        else:
          name = f"{name}"
        print(f"<<{pc:04x}:{op:2}: {self.dbg_name_for(pc):9}:{name:6}", end='')
        self._log_ds()
        print(">>")
      if self.RSDeep == 0 and op == RET:
        return
      if op in self.jumpTable:
        (self.jumpTable[op])()
      else:
        self(ERR_BAD_INSTRUCTION)
        return
    self.error(ERR_MAX_CYCLES)

  def _op_st(self, fn):
    """Apply operation λ(S,T), storing the result in S and dropping T"""
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    n = fn(self.S, self.T) & 0xffffffff
    self.S = (n & 0x7fffffff) - (n & 0x80000000)  # sign extend i32->whatever
    self.drop()

  def _op_t(self, fn):
    """Apply operation λ(T), storing the result in T"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    n = fn(self.T) & 0xffffffff
    self.T = (n & 0x7fffffff) - (n & 0x80000000)  # sign extend i32->whatever

  def _push(self, n):
    """Push n onto the data stack as a 32-bit signed integer"""
    deep = self.DSDeep
    if deep > 17:
      self.reset()             # Clear data and return stacks
      self.error(ERR_D_OVER)   # Set error code
      return
    if deep > 1:
      third = deep-2
      self.DStack[third] = self.S
    self.S = self.T
    self.T = n
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
      self.error(ERR_D_UNDER)
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax):
      self.error(ERR_BAD_ADDRESS)
    else:
      self.T = int.from_bytes(self.ram[addr:addr+1], 'little', signed=False)

  def load_byte_a(self):
    """Load byte from memory using address in register A"""
    addr = self.A
    if (addr < 0) or (addr > MemMax):
      self.error(ERR_BAD_ADDRESS)
      return
    x = int.from_bytes(self.ram[addr:addr+1], 'little', signed=False)
    self._push(x)

  def load_byte_b(self):
    """Load byte from memory using address in register B"""
    addr = self.B
    if (addr < 0) or (addr > MemMax):
      self.error(ERR_BAD_ADDRESS)
      return
    x = int.from_bytes(self.ram[addr:addr+1], 'little', signed=False)
    self._push(x)

  def load_byte_a_increment(self):
    """Load byte from memory using address in register A, then increment A"""
    addr = self.A
    if (addr < 0) or (addr > MemMax):
      self.error(ERR_BAD_ADDRESS)
      return
    x = int.from_bytes(self.ram[addr:addr+1], 'little', signed=False)
    self.A += 1
    self._push(x)

  def load_byte_b_increment(self):
    """Load byte from memory using address in register B, then increment B"""
    addr = self.B
    if (addr < 0) or (addr > MemMax):
      self.error(ERR_BAD_ADDRESS)
      return
    x = int.from_bytes(self.ram[addr:addr+1], 'little', signed=False)
    self.B += 1
    self._push(x)

  def store_byte_b_increment(self):
    """Store low byte of T byte to address in register B, then increment B"""
    addr = self.B
    x = int.to_bytes((self.T & 0xff), 1, 'little', signed=False)
    self.B += 1
    if (addr < 0) or (addr > MemMax) or (addr < self.Fence):
      self.error(ERR_BAD_ADDRESS)
    else:
      self.ram[addr:addr+1] = x
    self.drop()

  def store_byte(self):
    """Store low byte of S (uint8) at memory address T"""
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    x = int.to_bytes((self.S & 0xff), 1, 'little', signed=False)
    addr = self.T
    if (addr < 0) or (addr > MemMax) or (addr < self.Fence):
      self.error(ERR_BAD_ADDRESS)
    else:
      self.ram[addr:addr+1] = x
    self.drop()
    self.drop()

  def call(self):
    """Call to subroutine at address T, pushing old PC to return stack"""
    if self.RSDeep > 16:
      self.reset()
      self.error(ERR_R_OVER)
      return
    if self.T > MemMax:
      self.reset()
      self.error(ERR_BAD_ADDRESS)
      return
    # push the current Program Counter (PC) to return stack
    if self.RSDeep > 0:
      rSecond = self.RSDeep - 1
      self.RStack[rSecond] = self.R
    self.R = self.PC
    self.RSDeep += 1
    # set Program Counter to the new address
    self.PC = self.T
    self.drop()

  def jump_and_link(self):
    """Jump to subroutine after pushing old value of PC to return stack"""
    if self.RSDeep > 16:
      self.reset()
      self.error(ERR_R_OVER)
      return
    # read a 16-bit address from the instruction stream
    pc = self.PC
    n = (self.ram[pc+1] << 8) + self.ram[pc]  # decode little-endian halfword
    # push the current Program Counter (PC) to return stack
    if self.RSDeep > 0:
      rSecond = self.RSDeep - 1
      self.RStack[rSecond] = self.R
    self.R = pc + 2
    self.RSDeep += 1
    # set Program Counter to the new address
    self.PC = n

  def drop(self):
    """Drop T, the top item of the data stack"""
    deep = self.DSDeep
    if deep < 1:
      self.error(ERR_D_UNDER)
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
      self.error(ERR_D_UNDER)
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-3):
      self.error(ERR_BAD_ADDRESS)
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
    # set program counter to the new address
    self.PC = n

  def branch_zero(self):
    """Branch to address read from instruction stream if T == 0, drop T"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    pc = self.PC
    if self.T == 0:
      # Branch past conditional block: Set PC to address literal
      n = (self.ram[pc+1] << 8) + self.ram[pc]  # decode little-endian halfword
      self.PC = n
    else:
      # Enter conditional block: Advance PC past address literal
      self.PC += 2
    self.drop()

  def branch_for_loop(self):
    """Decrement R and branch to start of for-loop if R >= 0"""
    if self.RSDeep < 1:
      self.error(ERR_R_UNDER)
      return
    self.R -= 1
    pc = self.PC
    if self.R >= 0:
      # Keep looping: Set PC to address literal
      n = int.from_bytes(self.ram[pc:pc+2], 'little', signed=False)
      self.PC = n
    else:
      # End of loop: Advance PC past address literal, drop R
      self.PC += 2
      self.r_drop()

  def less_than(self):
    """Evaluate S < T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s < t else 0)

  def u16_literal(self):
    """Read uint16 halfword (2 bytes) literal, zero-extend it, push as T"""
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+2], 'little', signed=False)
    self.PC += 2
    self._push(n)

  def i32_literal(self):
    """Read int32 word (4 bytes) signed literal, push as T"""
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+4], 'little', signed=True)
    self.PC += 4
    self._push(n)

  def u8_literal(self):
    """Read uint8 byte literal, zero-extend it, push as T"""
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+1], 'little', signed=False)
    self.PC += 1
    self._push(n)

  def subtract(self):
    """Subtract T from S, store result in S, drop T"""
    self._op_st(lambda s, t: s - t)

  def decrement(self):
    """Subtract 1 from T"""
    self._op_t(lambda t: t - 1)

  def multiply(self):
    """Multiply S by T, store result in S, drop T"""
    self._op_st(lambda s, t: s * t)

  def not_equal(self):
    """Evaluate S != T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s != t else 0)

  def or_(self):
    """Store bitwise OR of S with T into S, then drop T"""
    self._op_st(lambda s, t: s | t)
    pass

  def over(self):
    """Push a copy of S"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    self._push(self.S)

  def add(self):
    """Add T to S, store result in S, drop T"""
    self._op_st(lambda s, t: s + t)

  def increment(self):
    """Add 1 to T"""
    self._op_t(lambda t: t + 1)

  def a_increment(self):
    """Add 1 to register A"""
    self.A += 1

  def a_decrement(self):
    """Subtract 1 from register A"""
    self.A -= 1

  def b_increment(self):
    """Add 1 to register B"""
    self.B += 1

  def b_decrement(self):
    """Subtract 1 from register B"""
    self.B -= 1

  def reset(self):
    """Reset the data stack, return stack and error code"""
    self.DSDeep = 0
    self.RSDeep = 0
    self.ERR = 0

  def set_fence(self):
    """Set the write protect fence to T, if T is greater than old fence"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    if self.T > self.Fence:
      self.Fence = self.T
    self.drop()

  def clear_err(self):
    """Clear the VM's error register (ERR)"""
    self.ERR = 0

  def return_(self):
    """Return from subroutine, taking address from return stack"""
    if self.RSDeep < 1:
      self.reset()
      self.error(ERR_R_UNDER)
      return
    # Set program counter from top of return stack
    self.PC = self.R
    # Drop top of return stack
    if self.RSDeep > 1:
      rSecond = self.RSDeep - 2
      self.R = self.RStack[rSecond]
    self.RSDeep -= 1

  def io_data_stack(self):
    self._log_ds(base=10)

  def io_return_stack(self):
    self._log_rs(base=10)

  def io_data_stack_hex(self):
    self._log_ds(base=16)

  def io_return_stack_hex(self):
    self._log_rs(base=16)

  def move_r_to_t(self):
    """Move top of return stack (R) to top of data stack (T)"""
    if self.RSDeep < 1:
      self.reset()
      self.error(ERR_R_UNDER)
      return
    if self.DSDeep > 17:
      self.reset()
      self.error(ERR_D_OVER)
      return
    self._push(self.R)
    if self.RSDeep > 1:
      rSecond = self.RSDeep - 2
      self.R = self.RStack[rSecond]
    self.RSDeep -= 1

  def r_drop(self):
    """Drop R in the manner needed when exiting from a counted loop"""
    if self.RSDeep < 1:
      self.reset()
      self.error(ERR_R_UNDER)
      return
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
      self.error(ERR_D_UNDER)
      return
    addr = self.T
    x = int.to_bytes(c_int32(self.S).value, 4, 'little', signed=True)
    if (addr < 0) or (addr > MemMax-3) or (addr < self.Fence):
      self.error(ERR_BAD_ADDRESS)
    else:
      self.ram[addr:addr+4] = x
    self.drop()
    self.drop()

  def swap(self):
    """Swap S with T"""
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    tmp = self.T
    self.T = self.S
    self.S = tmp

  def move_t_to_r(self):
    """Move top of data stack (T) to top of return stack (R)"""
    if self.RSDeep > 16:
      self.reset()
      self.error(ERR_R_OVER)
      return
    if self.DSDeep < 1:
      self.reset()
      self.error(ERR_D_UNDER)
      return
    if self.RSDeep > 0:
      rSecond = self.RSDeep - 1
      self.RStack[rSecond] = self.R
    self.R = self.T
    self.RSDeep += 1
    self.drop()

  def move_t_to_a(self):
    """Move top of data stack (T) to register A"""
    if self.DSDeep < 1:
      self.reset()
      self.error(ERR_D_UNDER)
      return
    self.A = self.T
    self.drop()

  def move_t_to_b(self):
    """Move top of data stack (T) to register B"""
    if self.DSDeep < 1:
      self.reset()
      self.error(ERR_D_UNDER)
      return
    self.B = self.T
    self.drop()

  def move_t_to_x(self):
    """Move top of data stack (T) to register X"""
    if self.DSDeep < 1:
      self.reset()
      self.error(ERR_D_UNDER)
      return
    self.X = self.T
    self.drop()

  def move_t_to_y(self):
    """Move top of data stack (T) to register Y"""
    if self.DSDeep < 1:
      self.reset()
      self.error(ERR_D_UNDER)
      return
    self.Y = self.T
    self.drop()

  def load_halfword(self):
    """Load halfword (2 bytes, zero-extended) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    addr = self.T
    if (addr < 0) or (addr > MemMax-1):
      self.error(ERR_BAD_ADDRESS)
      return
    self.T = int.from_bytes(self.ram[addr:addr+2], 'little', signed=False)

  def store_halfword(self):
    """Store low 2 bytes from S (uint16) at memory address T"""
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    x = int.to_bytes((self.S & 0xffff), 2, 'little', signed=False)
    addr = self.T
    if (addr < 0) or (addr > MemMax-1) or (addr < self.Fence):
      self.error(ERR_BAD_ADDRESS)
    else:
      self.ram[addr:addr+2] = x
    self.drop()
    self.drop()

  def xor(self):
    """Store bitwise XOR of S with T into S, then drop T"""
    self._op_st(lambda s, t: s ^ t)

  def zero_equal(self):
    """Evaluate 0 == T (true:-1, false:0), store result in T"""
    self._op_t(lambda t: -1 if 0 == t else 0)

  def true_(self):
    """Push -1 (true) to data stack"""
    self._push(-1)

  def false_(self):
    """Push 0 (false) to data stack"""
    self._push(0)

  def _log_ds(self, base=10):
    """Log (debug print) the data stack in the manner of .S"""
    print(" ", end='')
    deep = self.DSDeep
    if deep > 2:
      for i in range(deep-2):
        n = self.DStack[i]
        if base == 16:
          print(f" {n&0xffffffff:x}", end='')
        else:
          print(f" {n}", end='')
    if deep > 1:
      if base == 16:
        print(f" {self.S&0xffffffff:x}", end='')
      else:
        print(f" {self.S}", end='')
    if deep > 0:
      if base == 16:
        print(f" {self.T&0xffffffff:x}", end='')
      else:
        print(f" {self.T}", end='')
    else:
      print(" Stack is empty", end='')

  def _log_rs(self, base=10):
    """Log (debug print) the return stack in the manner of .S"""
    print(" ", end='')
    deep = self.RSDeep
    if deep > 1:
      for i in range(deep-1):
        n = self.RStack[i]
        if base == 16:
          print(f" {n&0xffffffff:x}", end='')
        else:
          print(f" {n}", end='')
    if deep > 0:
      if base == 16:
        print(f" {self.R&0xffffffff:x}", end='')
      else:
        print(f" {self.R}", end='')
    else:
      print(" R-Stack is empty", end='')

  def clear_error(self):
    """Clear VM error status code"""
    self.ERR = 0

  def r_(self):
    """Push a copy of top of Return stack (R) to the data stack"""
    self._push(self.R)

  def pc_(self):
    """Push a copy of the Program Counter (PC) to the data stack"""
    self._push(self.PC)

  def err_(self):
    """Push a copy of the Error register (ERR) to the data stack"""
    self._push(self.ERR)

  def a_(self):
    """Push a copy of register A to the data stack"""
    self._push(self.A)

  def b_(self):
    """Push a copy of register B to the data stack"""
    self._push(self.B)

  def x_(self):
    """Push a copy of register X to the data stack"""
    self._push(self.X)

  def y_(self):
    """Push a copy of register Y to the data stack"""
    self._push(self.Y)

  def io_key(self):
    """Push the next byte from Standard Input to the data stack.

    Input comes from readline, which is line buffered. So, this can block the
    event loop until a full line of text is available. The result of a call
    to `input()` is cached and returned byte by byte. Then, when that line has
    been completely consumed, `_read()` will make another call to `input()`.

    Results (stack effects):
    - Got an input byte, push 2 items: {S: byte, T: -1 (true)}
    - End of file, push 1 item:           {T: 0 (false)}
    """
    self.cycle_count = 65535  # reset the infinite loop detection count
    if len(self.inbuf) < 1:
      try:
        self.inbuf = input().encode('utf-8')+b'\x0a'
        if self.echo:
          # When stdin is coming from a pipe, echo input to stdout
          sys.stdout.flush()
          print(self.inbuf.decode('utf8')[:-1], end='')
          sys.stdout.flush()
      except EOFError:
        self.inbuf = b''
    if len(self.inbuf) > 0:
      self._push(self.inbuf[0])
      self._push(-1)
      self.inbuf = self.inbuf[1:]
    else:
      self._push(0)

  def io_emit(self):
    """Write low byte of T to the Standard Output stream.

    Doing the buffer.write() thing allows for writing utf-8 sequences byte by
    byte without having to buffer and parse UTF-8, or fight with Python's type
    system. This output method is most suitable for tests, demo code, and
    prototypes, where simplicity of the code is more important than its
    efficiency. Using this method to print long strings would be inefficient.
    """
    self.cycle_count = 65535  # reset the infinite loop detection count
    sys.stdout.flush()
    sys.stdout.buffer.write(int.to_bytes(self.T & 0xff, 1, 'little'))
    sys.stdout.flush()
    self.drop()

  def io_dot(self):
    """Print T to standard output, then drop T"""
    self.cycle_count = 65535  # reset the infinite loop detection count
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    sys.stdout.flush()
    print(f" {self.T}", end='')
    sys.stdout.flush()
    self.drop()

  def io_dump(self):
    """Hexdump S bytes of memory starting from address T, then drop S and T"""
    self.cycle_count = 65535  # reset the infinite loop detection count
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    if self.T > MemMax or self.T + self.S > MemMax:
      self.error(ERR_BAD_ADDRESS)
      return
    col = 0
    start = self.T
    end = self.T + self.S
    self.drop()
    self.drop()
    left = ""
    right = ""
    dirty = 0
    for (i, n) in enumerate(self.ram[start:end]):
      dirty = True
      if col == 0:                  # leftmost column is address
        left = f"{start+i:04x} "
        right = ""
      if col == 8:                  # extra space before 8th byte of a row
        left += " "
        right += " "
      left += f" {n:02x}"           # accumulate hex digits
      if n >= 32 and n <127:        # accumulate ASCII characters
        right += chr(n)
      else:
        right += '.'
      if col == 15:                 # print digits and chars at end of row
        print(f"{left}  {right}")
        dirty = False
      col = (col + 1) & 15
    if dirty:
      # if number of bytes requested was not an even multiple of 16, then
      # print what's left of the last row
      print(f"{left}  {right}")

  def trace_on(self):
    """Enable debug tracing (also see DEBUG global var)"""
    self.dbg_trace_enable = True

  def trace_off(self):
    """Disable debug tracing (also see DEBUG global var)"""
    self.dbg_trace_enable = False

  def _ok_or_err(self):
    """Print the OK or ERR line-end status message and clear any errors"""
    if self.ERR != 0:
      print(f"  ERR{self.ERR}")
      self.ERR = 0
    else:
      print("  OK")

"""
Load and boot the ROM file when VM is run as a module rather than imported
"""
if __name__ == '__main__':

  # Start by assuming we'll use the default rom file
  rom = ROM_FILE

  # Then check for a command line argument asking for a different rom.
  # For example: `./markab_vm.py hello.rom`
  if (len(sys.argv) == 2) and (sys.argv[1].endswith(".rom")):
    rom = sys.argv[1]

  # Make a VM instance
  v = VM(echo=(not sys.stdin.isatty()))

  # Attempt to load debug symbols (e.g. for kernel.rom, check kernel.symbols)
  sym_file = rom[:-4] + ".symbols"
  v.sym_addrs = []
  v.sym_names = []
  try:
    with open(sym_file, 'r') as f:
      lines = f.read().strip().split("\n")
      lines = [L.split() for L in lines]
      lines.reverse()
      for (addr, name) in lines:
        v.dbg_add_symbol(addr, name)
  except:
    pass

  # Open the rom file and run it
  with open(rom, 'rb') as f:
    rom = f.read()
    if PROFILE:
      # This will print a chart of function call counts and timings
      cProfile.run('v._warm_boot(rom, 65535)', sort='cumulative')
    else:
      v._warm_boot(rom, 65535)

  if v.echo:
    # add a final newline if input is coming from pipe
    print()
