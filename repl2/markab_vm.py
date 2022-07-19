#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab VM emulator
#
import asyncio
import cProfile
from ctypes import c_int32
import os
import re
import readline
import sys
import time
from typing import Callable, Dict

from mkb_autogen import (
  NOP, ADD, SUB, INC, DEC, MUL, DIV, MOD, AND, INV, OR, XOR, SLL, SRL, SRA,
  EQ, GT, LT, NE, ZE, TRUE, FALSE, JMP, JAL, CALL, RET, HALT,
  BZ, BFOR, MTR, RDROP, R, PC, ERR, MTE, DROP, DUP, OVER, SWAP,
  U8, U16, I32, LB, SB, LH, SH, LW, SW, RESET,
  IOD, IODH, IORH, IOKEY, IOEMIT, IODOT, IODUMP, IOLOAD, IOSAVE, TRON, TROFF,
  MTA, LBA, LBAI,       AINC, ADEC, A,
  MTB, LBB, LBBI, SBBI, BINC, BDEC, B,

  Heap, HeapRes, HeapMax, MemMax, IRQRX, ErrUnknown, ErrNest,
  OPCODES,
)
from mkb_irc import Irc


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

# Allow list of regular expressions for files that can be written by IOSAVE
IOSAVE_ALLOW_RE_LIST = """
self_hosted\.rom
"""

# Allow list of regular expressions for files that can be read by IOLOAD
IOLOAD_ALLOW_RE_LIST = """
.+\.mkb
"""

ROM_FILE = 'kernel.rom'

ERR_D_OVER = 1            #  1: Data stack overflow
ERR_D_UNDER = 2           #  2: Data stack underflow
ERR_BAD_ADDRESS = 3       #  3: Expected vaild address but got something else
ERR_BOOT_OVERFLOW = 4     #  4: ROM image is too big to fit in the heap
ERR_BAD_INSTRUCTION = 5   #  5: Expected an opcode but got something else
ERR_R_OVER = 6            #  6: Return stack overflow
ERR_R_UNDER = 7           #  7: Return stack underflow
ERR_MAX_CYCLES = 8        #  8: Call to _step() ran for too many clock cycles
ERR_FILE_PERMS = 9        #  9: In `load" x"`: x failed permissions check
ERR_FILE_NOT_FOUND = 10   # 10: In `load" x"`: opening x failed
ERR_UNKNOWN = ErrUnknown  # 11: Outer interpreter encountered an unknown word
ERR_NEST = ErrNest        # 12: Compiler encountered unbalanced }if or }for
ERR_IOLOAD_DEPTH = 13     # 13: Too many levels of nested `load" ..."` calls
ERR_BAD_PC_ADDR = 14      # 14: Bad program counter value: address not in heap

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
    self.T = 0                      # Top of data stack
    self.S = 0                      # Second on data stack
    self.R = 0                      # top of Return stack
    self.PC = Heap                  # Program Counter
    self.DSDeep = 0                 # Data Stack Depth (count include T and S)
    self.RSDeep = 0                 # Return Stack Depth (count inlcudes R)
    self.DStack = [0] * 16          # Data Stack
    self.RStack = [0] * 16          # Return Stack
    self.ram = bytearray(MemMax+1)  # Random Access Memory
    self.inbuf = b''                # Input buffer
    self.outbuf = b''               # Output buffer
    self.echo = echo                # Echo depends on tty vs pip, etc.
    self.cycle_count = 65535        # Counter to break infinite loops
    self.halted = False             # Flag to track halt (used for `bye`)
    self.stdout_irq = None          # IRQ line (callback) for buffering stdout
    self.ioload_depth = 0           # Nesting level for io_load_file
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
    self.jumpTable[DIV  ] = self.divide
    self.jumpTable[MOD  ] = self.modulo
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
    self.jumpTable[HALT ] = self.halt
    self.jumpTable[BZ   ] = self.branch_zero
    self.jumpTable[BFOR ] = self.branch_for_loop
    self.jumpTable[MTR  ] = self.move_t_to_r
    self.jumpTable[RDROP] = self.r_drop
    self.jumpTable[R    ] = self.r_
    self.jumpTable[PC   ] = self.pc_
    self.jumpTable[ERR  ] = self.err_
    self.jumpTable[MTE  ] = self.move_t_to_err
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
    self.jumpTable[IOD  ] = self.io_data_stack
    self.jumpTable[IODH ] = self.io_data_stack_hex
    self.jumpTable[IORH ] = self.io_return_stack_hex
    self.jumpTable[IOKEY] = self.io_key
    self.jumpTable[IOEMIT] = self.io_emit
    self.jumpTable[IODOT] = self.io_dot
    self.jumpTable[IODUMP] = self.io_dump
    self.jumpTable[IOLOAD] = self.io_load_file
    self.jumpTable[IOSAVE] = self.io_save_file
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
    # Compile filename allow-list regular expressions for IOLOAD
    self.ioload_allow_re = []
    for line in IOLOAD_ALLOW_RE_LIST.strip().split("\n"):
      self.ioload_allow_re.append(re.compile(line))
    # Compile filename allow-list regular expressions for IOSAVE
    self.iosave_allow_re = []
    for line in IOSAVE_ALLOW_RE_LIST.strip().split("\n"):
      self.iosave_allow_re.append(re.compile(line))

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

  def set_stdout_irq(self, stdout_irq_fn: Callable[None, None]):
    """Set callback function to raise interrupt line for available output.
    This is a little weird because of my desire to avoid building a bunch of
    `async` and `await` stuff into the VM class. This arrangement allows for
    an async function to provide the VM with input and ask it for pending
    output after the stdout_irq signal is sent.
    """
    self.stdout_irq = stdout_irq_fn

  def print(self, *args, **kwargs):
    """Send output to stdout (terminal mode) or buffer it (irq mode) """
    if self.stdout_irq is None:
      # In termio mode send output to stdout
      print(self.outbuf.decode('utf8'), end='')
      self.outbuf = b''
      print(*args, **kwargs)
      sys.stdout.flush()
    else:
      # When using stdout_irq, buffer the output and raise the IRQ line
      end = kwargs.get('end', "\n").encode('utf8')
      new_stuff = (" ".join(args)).encode('utf8')
      new_stuff += end
      self.outbuf += new_stuff
      self.stdout_irq()

  def drain_stdout(self):
    """ """
    outbuf = self.outbuf.decode('utf8')
    self.outbuf = b''
    return outbuf

  def error(self, code):
    """Set the ERR register with an error code"""
    if DEBUG:
      self.print(f"<<ERR{code}>>", end='')
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

  def irq_rx(self, line):
    """Interrupt request for receiving a line of input"""
    # First, copy the input line to the input buffer as UTF-8
    self.inbuf += line.encode('utf8')
    # Be sure the line ends with LF (some input sources do, some don't)
    if line[-1] != "\n":
      self.inbuf += b'\x0a'
    # When stdin is a pipe, echo input to stdout (minus its trailing newline)
    if (not self.stdout_irq) and self.echo:
      print(line[:-1], end='')
    # Next, attempt to jump to interrupt request vector for received input
    self._push(IRQRX)
    self.load_halfword()
    irq_vector = self.T
    self.drop()
    # allow for the possibility that the interrupt vector is not set
    if irq_vector != 0:
      self.PC = irq_vector
      max_cycles = 65335
      self._step(max_cycles)
    else:
      if DEBUG:
        print("<< IRQRX is 0 >>>")

  def _step(self, max_cycles):
    """Step the virtual CPU clock to run up to max_cycles instructions"""
    self.cycle_count = max_cycles
    while self.cycle_count > 0:
      self.cycle_count -= 1
      pc = self.PC
      if pc < 0 or pc >= HeapMax:
        # Stop if program counter somehow got set to address not in dictionary
        self.error(ERR_BAD_PC_ADDR)
        self.print(f"  ERR {self.ERR}")
        return
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
        if DEBUG:
          print("\n========================")
          print("DStack: ", end='')
          self.io_data_stack()
          print("\nRStack: ", end='')
          self.io_return_stack_hex()
          print(f"\nPC: 0x{self.PC:04x} = {self.PC}")
          raise Exception("bad instruction", op)
        self.error(ERR_BAD_INSTRUCTION)
        self.print(f"  ERR {self.ERR}")
        return
    # Making it here means the VM interrupted the code because it ran too long.
    # This probably means things have gone haywire, and potentially the memory
    # state is corrupted. But, try to make the best of it. So, since the kernel
    # didn't get a chance to print its own prompt, set the error code and
    # print an error prompt. Also, clear the return stack
    self.error(ERR_MAX_CYCLES)
    self.print(f"  ERR {self.ERR}")
    self.RSDeep = 0


  def _op_st(self, fn):
    """Apply operation λ(S,T), storing the result in S and dropping T"""
    deep = self.DSDeep
    if deep < 2:
      self.error(ERR_D_UNDER)
      return
    n = fn(self.S, self.T) & 0xffffffff
    self.T = (n & 0x7fffffff) - (n & 0x80000000)  # sign extend i32->whatever
    # The rest of this function is an unroll of a call to drop() with a slight
    # optimization to avoid copying S to T. According to cProfile, _op_st() is
    # one of the most frequently called functions in the VM. Avoiding the extra
    # function call here speeds it up measurably.
    if deep > 2:
      third = deep - 3
      self.S = self.DStack[third]
    self.DSDeep = deep - 1

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
    addr = self.T & 0xffff
    self.T = self.ram[addr]

  def load_byte_a(self):
    """Load byte from memory using address in register A"""
    addr = self.A & 0xffff
    x = self.ram[addr]
    self._push(x)

  def load_byte_b(self):
    """Load byte from memory using address in register B"""
    addr = self.B & 0xffff
    x = self.ram[addr]
    self._push(x)

  def load_byte_a_increment(self):
    """Load byte from memory using address in register A, then increment A"""
    addr = self.A & 0xffff
    x = self.ram[addr]
    self.A += 1
    self._push(x)

  def load_byte_b_increment(self):
    """Load byte from memory using address in register B, then increment B"""
    addr = self.B & 0xffff
    x = self.ram[addr]
    self.B += 1
    self._push(x)

  def store_byte_b_increment(self):
    """Store low byte of T byte to address in register B, then increment B"""
    addr = self.B & 0xffff
    self.B += 1
    self.ram[addr] = self.T & 0xff
    self.drop()

  def store_byte(self):
    """Store low byte of S (uint8) at memory address T"""
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    addr = self.T & 0xffff
    self.ram[addr] = self.S & 0xff
    self.drop()
    self.drop()

  def call(self):
    """Call to subroutine at address T, pushing old PC to return stack"""
    if self.RSDeep > 16:
      self.reset()
      self.error(ERR_R_OVER)
      return
    # push the current Program Counter (PC) to return stack
    if self.RSDeep > 0:
      rSecond = self.RSDeep - 1
      self.RStack[rSecond] = self.R
    self.R = self.PC
    self.RSDeep += 1
    # set Program Counter to the new address
    self.PC = self.T & 0xffff
    self.drop()

  def jump_and_link(self):
    """Jump to subroutine after pushing old value of PC to return stack.
    The jump address is PC-relative to allow for relocatable object code.
    """
    if self.RSDeep > 16:
      self.reset()
      self.error(ERR_R_OVER)
      return
    # read a 16-bit signed offset (relative to PC) from instruction stream
    pc = self.PC
    n = (self.ram[pc+1] << 8) + self.ram[pc]  # decode little-endian halfword
    n = (n & 0x7fff) - (n & 0x8000)           # sign extend it
    # push the current Program Counter (PC) to return stack
    if self.RSDeep > 0:
      rSecond = self.RSDeep - 1
      self.RStack[rSecond] = self.R
    self.R = pc + 2
    self.RSDeep += 1
    # add offset to program counter to compute destination address.
    # the 0xffff mask lets you do stuff like (5-100) & 0xffff = 65441 so a
    # a signed 16-bit pc-relative offset can address the full memory range
    self.PC = (pc + n) & 0xffff

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
    deep = self.DSDeep
    if deep < 2:
      self.error(ERR_D_UNDER)
      return
    self.T = -1 if (self.S == self.T) else 0
    # The rest of this function is an unroll of a call to drop() with a slight
    # optimization to avoid copying S to T. According to cProfile, equal() is
    # one of the most frequently called functions in the VM. Avoiding the extra
    # function call here speeds it up measurably.
    if deep < 1:
      self.error(ERR_D_UNDER)
      return
    if deep > 2:
      third = deep - 3
      self.S = self.DStack[third]
    self.DSDeep = deep - 1

  def load_word(self):
    """Load a signed int32 (word = 4 bytes) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    addr = self.T & 0xffff
    if addr > MemMax-3:
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
    """Jump to subroutine at address read from instruction stream.
    The jump address is PC-relative to allow for relocatable object code.
    """
    # read a 16-bit PC-relative address offset from the instruction stream
    pc = self.PC
    n = (self.ram[pc+1] << 8 ) | self.ram[pc]  # LE halfword
    n = (n & 0x7fff) - (n & 0x8000)           # sign extend it
    # add offset to program counter to compute destination address.
    # the 0xffff mask lets you do stuff like (5-100) & 0xffff = 65441 so a
    # a signed 16-bit pc-relative offset can address the full memory range
    self.PC = (self.PC + n) & 0xffff

  def branch_zero(self):
    """Branch to PC-relative address if T == 0, drop T.
    The branch address is PC-relative to allow for relocatable object code.
    """
    deep = self.DSDeep
    if deep < 1:
      self.error(ERR_D_UNDER)
      return
    pc = self.PC
    if self.T == 0:
      # Branch forward past conditional block: Add address literal from
      # instruction stream to PC. Maximum branch distance is +255.
      self.PC = pc + self.ram[pc]
    else:
      # Enter conditional block: Advance PC past address literal
      self.PC = pc + 1
    # The rest of this function is an unroll of a call to drop(). According to
    # cProfile, branch_zero() is one of the most frequently called functions in
    # the VM. Avoiding the extra function call here speeds it up measurably.
    self.T = self.S
    if deep > 2:
      third = deep - 3
      self.S = self.DStack[third]
    self.DSDeep = deep - 1

  def branch_for_loop(self):
    """Decrement R and branch to start of for-loop if R >= 0.
    The branch address is PC-relative to allow for relocatable object code.
    """
    if self.RSDeep < 1:
      self.error(ERR_R_UNDER)
      return
    self.R -= 1
    pc = self.PC
    if self.R >= 0:
      # Keep looping: Branch backwards by subtracting byte literal from PC
      # Maximum branch distance is -255
      self.PC -= self.ram[pc]
    else:
      # End of loop: Advance PC past address literal, drop R
      self.PC += 1
      self.r_drop()

  def less_than(self):
    """Evaluate S < T (true:-1, false:0), store result in S, drop T"""
    self._op_st(lambda s, t: -1 if s < t else 0)

  def u16_literal(self):
    """Read uint16 halfword (2 bytes) literal, zero-extend it, push as T"""
    pc = self.PC
    n = (self.ram[pc+1] << 8) | self.ram[pc]  # LE halfword
    self.PC += 2
    # The rest of this function is an unroll of _push(). The point is to save
    # the overhead of a function call since U16 gets called very frequently.
    deep = self.DSDeep
    if deep > 17:
      self.reset()             # Clear data and return stacks
      self.error(ERR_D_OVER)   # Set error code
      return
    if deep > 1:
      third = deep - 2
      self.DStack[third] = self.S
    self.S = self.T
    self.T = n
    self.DSDeep = deep + 1

  def i32_literal(self):
    """Read int32 word (4 bytes) signed literal, push as T"""
    pc = self.PC
    n = int.from_bytes(self.ram[pc:pc+4], 'little', signed=True)
    self.PC += 4
    self._push(n)

  def u8_literal(self):
    """Read uint8 byte literal, zero-extend it, push as T"""
    pc = self.PC
    n = self.ram[pc]
    self.PC = pc + 1
    # The rest of this function is an unroll of _push(). The point is to save
    # the overhead of a function call since U8 gets called very frequently.
    deep = self.DSDeep
    if deep > 17:
      self.reset()             # Clear data and return stacks
      self.error(ERR_D_OVER)   # Set error code
      return
    if deep > 1:
      third = deep - 2
      self.DStack[third] = self.S
    self.S = self.T
    self.T = n
    self.DSDeep = deep + 1

  def subtract(self):
    """Subtract T from S, store result in S, drop T"""
    self._op_st(lambda s, t: s - t)

  def decrement(self):
    """Subtract 1 from T"""
    self._op_t(lambda t: t - 1)

  def multiply(self):
    """Multiply S by T, store result in S, drop T"""
    self._op_st(lambda s, t: s * t)

  def divide(self):
    """Divide S by T (integer division), store quotient in S, drop T"""
    self._op_st(lambda s, t: s // t)

  def modulo(self):
    """Divide S by T (integer division), store remainder in S, drop T"""
    self._op_st(lambda s, t: s % t)

  def not_equal(self):
    """Evaluate S != T (true:-1, false:0), store result in S, drop T"""
    deep = self.DSDeep
    if deep < 2:
      self.error(ERR_D_UNDER)
      return
    self.T = -1 if (self.S != self.T) else 0
    # The rest of this function is an optimized unroll of a call to drop()
    if deep > 2:
      third = deep - 3
      self.S = self.DStack[third]
    self.DSDeep = deep - 1

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

  def halt(self):
    """Set the halt flag to stop further instructions (used for `bye`)"""
    self.halted = True

  def io_data_stack(self):
    self._log_ds(base=10)

  def io_data_stack_hex(self):
    self._log_ds(base=16)

  def io_return_stack_hex(self):
    self._log_rs(base=16)

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
    addr = self.T & 0xffff
    x = int.to_bytes(c_int32(self.S).value, 4, 'little', signed=True)
    if addr > MemMax-3:
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

  def move_t_to_err(self):
    """Move top of data stack (T) to ERR register"""
    if self.DSDeep < 1:
      self.reset()
      self.error(ERR_D_UNDER)
      return
    self.ERR = self.T
    self.drop()

  def load_halfword(self):
    """Load halfword (2 bytes, zero-extended) from memory address T, into T"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    addr = self.T & 0xffff
    if addr > MemMax-1:
      self.error(ERR_BAD_ADDRESS)
      return
    self.T = (self.ram[addr+1] << 8) | self.ram[addr]  # LE halfword

  def store_halfword(self):
    """Store low 2 bytes from S (uint16) at memory address T"""
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    addr = self.T & 0xffff
    if addr > MemMax-1:
      self.error(ERR_BAD_ADDRESS)
    else:
      self.ram[addr] = self.S & 0xff           # LE halfword low byte
      self.ram[addr+1] = (self.S >> 8) & 0xff  # LE halfword high byte
    self.drop()
    self.drop()

  def xor(self):
    """Store bitwise XOR of S with T into S, then drop T"""
    self._op_st(lambda s, t: s ^ t)

  def zero_equal(self):
    """Evaluate 0 == T (true:-1, false:0), store result in T"""
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    self.T = -1 if (self.T == 0) else 0

  def true_(self):
    """Push -1 (true) to data stack"""
    self._push(-1)

  def false_(self):
    """Push 0 (false) to data stack"""
    self._push(0)

  def _log_ds(self, base=10):
    """Log (debug print) the data stack in the manner of .S"""
    buf = " "
    deep = self.DSDeep
    if deep > 2:
      for i in range(deep-2):
        n = self.DStack[i]
        if base == 16:
          buf += f" {n&0xffffffff:x}"
        else:
          buf += f" {n}"
    if deep > 1:
      if base == 16:
        buf += f" {self.S&0xffffffff:x}"
      else:
        buf += f" {self.S}"
    if deep > 0:
      if base == 16:
        buf += f" {self.T&0xffffffff:x}"
      else:
        buf += f" {self.T}"
    else:
      buf += " Stack is empty"
    self.print(buf, end='')

  def _log_rs(self, base=10):
    """Log (debug print) the return stack in the manner of .S"""
    buf = " "
    deep = self.RSDeep
    if deep > 1:
      for i in range(deep-1):
        n = self.RStack[i]
        if base == 16:
          buf += f" {n&0xffffffff:x}"
        else:
          buf += f" {n}"
    if deep > 0:
      if base == 16:
        buf += f" {self.R&0xffffffff:x}"
      else:
        buf += f" {self.R}"
    else:
      buf += " R-Stack is empty"
    self.print(buf, end='')

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

  def io_key(self):
    """Push the next byte from Standard Input to the data stack.

    Results (stack effects):
    - Got an input byte, push 2 items: {S: byte, T: -1 (true)}
    - End of file, push 1 item:           {T: 0 (false)}
    """
    self.cycle_count = 65535  # reset the infinite loop detection count
    if len(self.inbuf) > 0:
      self._push(self.inbuf[0])
      self._push(-1)
      self.inbuf = self.inbuf[1:]
    else:
      self._push(0)

  def io_emit(self):
    """Buffer the low byte of T for stdout.

    This is meant to allow for utf-8 sequences to be emitted 1 byte at a time
    without getting into fights with Python over its expectations about string
    encoding. This output method is most suitable for tests, demo code, and
    prototypes, where simplicity of the code is more important than its
    efficiency. Using this method to print long strings would be inefficient.
    """
    self.cycle_count = 65535  # reset the infinite loop detection count
    new_stuff = int.to_bytes(self.T & 0xff, 1, 'little')
    self.outbuf += new_stuff
    if self.T == 10:
      self.print(end='')
    self.drop()

  def io_dot(self):
    """Print T to standard output, then drop T"""
    self.cycle_count = 65535  # reset the infinite loop detection count
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    new_stuff = f" {self.T}".encode('utf8')
    self.outbuf += new_stuff
    self.drop()

  def io_dump(self):
    """Hexdump S bytes of memory starting from address T, then drop S and T"""
    self.cycle_count = 65535  # reset the infinite loop detection count
    if self.DSDeep < 2:
      self.error(ERR_D_UNDER)
      return
    bad_start_addr = (self.T < 0) or (self.T > MemMax)
    bad_byte_count = (self.S < 0) or (self.T + self.S > MemMax)
    if bad_start_addr or bad_byte_count:
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
        left = f"{start+i:04x}  "
        right = ""
      if col in [4, 8, 12]:         # space before 4th, 8th, 12th bytes of row
        left += " "
        right += " "
      left += f"{n:02x}"            # accumulate hex digits
      if n >= 32 and n <127:        # accumulate ASCII characters
        right += chr(n)
      else:
        right += '.'
      if col == 15:                 # print digits and chars at end of row
        self.print(f"{left}  {right}")
        dirty = False
      col = (col + 1) & 15
    if dirty:
      # if number of bytes requested was not an even multiple of 16, then
      # print what's left of the last row
      self.print(f"{left:41}  {right}")

  # =========================================================================
  # =============== START OF DANGEROUS FILE IO STUFF ========================
  # =========================================================================
  # ==                                                                     ==
  # ==  As far as I know, this is code is implemented well, but file IO    ==
  # ==  is traditionally a common area for mistakes leading to unintended  ==
  # ==  behavior. It's possible I've overlooked something important.       ==
  # ==                                                                     ==
  # ==  The safeguards here are mainly designed to prevent file IO by      ==
  # ==  accident in the event of typos or bugs. If you, the random future  ==
  # ==  reader of this code, feel adventurous and get inspired to run      ==
  # ==  this VM on a public irc channel, bad things might happen. Don't    ==
  # ==  say I never warned you. (maybe check out the allow lists up top)   ==
  # ==                                                                     ==
  # =========================================================================

  # ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
  # If you are reviewing for possible security issues, pay attention here
  def _load_string(self, addr):
    """Load a Markab string from ram[addr] and return it as a python string"""
    addr = self.T & 0xffff
    count = self.ram[addr]
    if addr + count < len(self.ram):
      s = (self.ram[addr+1:addr+count+1]).decode('utf8')
      return s
    else:
      if DEBUG:
        print("<<< _load_string(): bad address >>>")
      return ""

  # ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
  # If you are reviewing for possible security issues, pay attention here
  def _normalize_filepath(self, filepath):
    path = os.path.normcase(os.path.abspath(filepath))
    if DEBUG:
      print(f"<<< _normalize_filepath() -> '{path}' >>>")
    return path

  # ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
  # If you are reviewing for possible security issues, pay attention here
  def _is_file_in_cwd(self, filepath):
    """Return whether file path is in the current working directory"""
    cwd = self._normalize_filepath(os.getcwd())
    abs_path = self._normalize_filepath(filepath)
    if abs_path.startswith(cwd):
      return True
    else:
      return False

  # ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
  # If you are reviewing for possible security issues, pay attention here
  def io_load_file(self):
    """Load and interpret file, taking file path from string pointer in T.
    File path must pass two tests:
    1. Match one of the allow-list regular expressions for IOLOAD
    2. Be located in the current working directory (including subdirectories)
    """
    if self.DSDeep < 1:
      self.error(ERR_D_UNDER)
      return
    if self.ioload_depth > 1:
      self.error(ERR_IOLOAD_DEPTH)
      return
    filepath = self._load_string(self.T)
    self.drop()
    # Check filename against the allow list
    re_match = any([a.match(filepath) for a in self.ioload_allow_re])
    cwd_match = self._is_file_in_cwd(filepath)
    if DEBUG:
      print(f"<<< IOLOAD: re match '{filepath}' ? --> {re_match} >>>")
      print(f"<<< IOLOAD: cwd match '{filepath}' ? --> {cwd_match} >>>")
    if (not re_match) or (not cwd_match):
      self.error(ERR_FILE_PERMS)
      return
    try:
      # Save VM state
      old_inbuf = self.inbuf
      old_pc = self.PC
      old_echo = self.echo
      # Read and interpret the file
      self.echo = False
      with open(filepath) as f:
        self.ioload_depth += 1
        for (n, line) in enumerate(f):
          self.irq_rx(line)
          if self.ERR != 0:
            line = line.split("\n")[0]
            self.print(f"ERR {filepath}:{n+1}: {line}")
            break
      # Restore old state
      self.ioload_depth -= 1
      self.PC = old_pc
      self.echo = old_echo
      if self.ERR == 0:
        self.inbuf = old_inbuf
    except FileNotFoundError:
      self.error(ERR_FILE_NOT_FOUND)
      return

  # ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
  # If you are reviewing for possible security issues, pay attention here
  def io_save_file(self):
    """Save memory to file, T: filename, S: source address, 3rd: byte count.
    File path must pass two tests:
    1. Match one of the allow-list regular expressions for IOSAVE
    2. Be located in the current working directory (including subdirectories)
    """
    # //////////////////////////////////////////////////////////////////
    print("//////// TODO FINISH IMPLEMENTING io_save_file() ///////////")
    # //////////////////////////////////////////////////////////////////
    if self.DSDeep < 3:
      self.error(ERR_D_UNDER)
      return
    # Pop arguments off stack before doing any error checks
    filepath = self._load_string(self.T)
    self.drop()
    src_addr = self.T & 0xffff
    self.drop()
    count = self.T & 0xffff
    self.drop()
    # Check the filename against the allow list
    re_match = any([a.match(filepath) for a in self.iosave_allow_re])
    cwd_match = self._is_file_in_cwd(filepath)
    if DEBUG:
      print(f"<<< IOSAVE: re match '{filepath}' ? --> {re_match} >>>")
      print(f"<<< IOSAVE: cwd match '{filepath}' ? --> {cwd_match} >>>")
    if (not re_match) or (not cwd_match):
      self.error(ERR_FILE_PERMS)
      return
    # Check if source address and byte count are reasonable
    if src_addr + count > len(self.ram):
      self.error(ERR_BAD_ADDRESS)
      return
    #///////////////////////////////////////////
    # TODO: make this part actually save a file
    #///////////////////////////////////////////

  # =======================================================================
  # === END OF DANGEROUS FILE IO STUFF ====================================
  # =======================================================================

  def trace_on(self):
    """Enable debug tracing (also see DEBUG global var)"""
    self.dbg_trace_enable = True

  def trace_off(self):
    """Disable debug tracing (also see DEBUG global var)"""
    self.dbg_trace_enable = False

  def _ok_or_err(self):
    """Print the OK or ERR line-end status message and clear any errors"""
    if self.ERR != 0:
      self.print(f"  ERR {self.ERR}")
      self.ERR = 0
    else:
      self.print("  OK")


class Irq():
  """Class to manage virtual interrupt requests between async/non-async"""
  def __init__(self, vm, irc):
    self.vm = vm
    self.irc = irc
    self.stdout_interrupt = False
    self.stdin_interrupt = False

  def stdout(self):
    """Non-async callback so the VM can signal it has buffered output ready"""
    self.stdout_interrupt = True

  async def drain_stdout(self):
    """Async interrupt handler that can be used with Irc.listen()"""
    if self.stdout_interrupt:
      stdout_buf = self.vm.drain_stdout().strip()
      for line in stdout_buf.split("\n"):
        await self.irc.notice(line)
      self.stdout_interrupt = False


async def irc_main(vm, rom_bytes, max_cycles):
  """Start the VM in irc-bot mode"""
  nick = 'mkbot'
  name = 'mkbot'
  host = 'localhost'         # connecting from localhost
  irc_server = 'localhost'   # ...to ngircd server also on localhost
  irc_port = 6667
  chan = '#mkb'

  # Plumb up interrupt handling and stdin/stdout between VM and irc
  irc = Irc(nick, name, host, irc_server, irc_port, chan)
  irq = Irq(vm, irc)
  irc.set_rx_callback(vm.irq_rx)
  irc.set_rx_irq(irq.drain_stdout)
  vm.set_stdout_irq(irq.stdout)

  # Connect to irc
  await irc.connect()
  await irc.join()
  vm._warm_boot(rom_bytes, max_cycles)  # this should return quickly
  await irc.listen()                    # this is the REPL event loop

def termio_boot(vm, rom_bytes, max_cycles):
  """VM bootload and input loop for terminal mode"""
  vm._warm_boot(rom_bytes, max_cycles)
  vm._push(IRQRX)
  vm.load_halfword()
  if vm.T == 0:
    # if boot code did not set a receive IRQ vector, don't start input loop
    return
  vm.drop()
  for line in sys.stdin:
    # Input comes from readline, which is line buffered, so this blocks the
    # thread until a full line of text is available. The VM is expected to
    # process the line of input and then return promptly.
    vm.irq_rx(line)
    if vm.halted:
      # this happens when `bye` signals it wants to exit
      break

def termio_main(vm, rom_bytes, max_cycles):
  """Start the VM in terminal IO mode"""
  if PROFILE:
    # This will print a chart of function call counts and timings at exit
    cProfile.run('termio_boot(v, rom_bytes, 65535)', sort='cumulative')
  else:
    termio_boot(v, rom_bytes, max_cycles)

  if v.echo:
    # add a final newline if input is coming from pipe
    print()


"""
Load and boot the ROM file when VM is run as a module rather than imported
"""
if __name__ == '__main__':

  # Start by assuming we'll use the default rom file and terminal IO
  rom = ROM_FILE
  irc_io = False
  args = sys.argv[1:]

  # Check for a command line argument asking for a different rom.
  # For example: `./markab_vm.py hello.rom`
  if (len(args) > 0) and (args[-1].endswith(".rom")):
      rom = args[-1]

  # Check for `--irc` flag asking to use irc for IO
  if (len(args) > 0) and '--irc' in args:
    irc_io = True

  # Make a VM instance
  v = VM(echo=(not sys.stdin.isatty()))

  # Attempt to load debug symbols (e.g. for kernel.rom, check kernel.symbols)
  v.sym_addrs = []
  v.sym_names = []
  if DEBUG:
    sym_file = rom[:-4] + ".symbols"
    try:
      with open(sym_file, 'r') as f:
        lines = f.read().strip().split("\n")
        lines = [L.split() for L in lines]
        lines.reverse()
        for (addr, name) in lines:
          v.dbg_add_symbol(addr, name)
    except:
      pass

  # Load the rom file
  rom_bytes = b''
  with open(rom, 'rb') as f:
    rom_bytes = f.read()

  # Boot the VM
  if irc_io:
    # Start in irc bot IO mode
    asyncio.run(irc_main(v, rom_bytes, 65535))
  else:
    termio_main(v, rom_bytes, 65535)
