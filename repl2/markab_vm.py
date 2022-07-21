#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
"""
Markab VM emulator

Emulate CPU, RAM, and peripherals for a Markab virtual machine instance.
"""
from ctypes import c_int32
import os
import re
import sys
import time
from typing import Callable, Dict

import mkb_autogen as ag
from mkb_autogen import MemMax, IRQRX, IRQERR, ErrUnknown, ErrNest, OPCODES

# This controls debug tracing and dissasembly. DEBUG is a global enable for
# tracing, but it won't do anything unless the code uses a TRON instruction to
# turn tracing on (TROFF turns it back off). Intended usage is that you bracket
# a specific area of code to be traced with TRON/TROFF. Otherwise, the huge
# amount of trace data from dictionary lookups will be too noisy to follow.
#
DEBUG = False #True

# Set this to True if you want a profile report from cProfile
#
PROFILE = False #True

if PROFILE:
  import cProfile


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
ERR_IOLOAD_FAIL = 15      # 15: Error while loading a file

# Configure STDIN/STDOUT at load-time for use utf-8 encoding.
# For documentation on arguments to `reconfigure()`, see
# https://docs.python.org/3/library/io.html#io.TextIOWrapper
sys.stdout.reconfigure(encoding='utf-8', line_buffering=True)
sys.stdin.reconfigure(encoding='utf-8', line_buffering=True)

# VM state variables: model the registers and RAM of a virtual CPU
ERR = 0                    # Error register (don't confuse with ERR opcode!)
BASE = 10                  # number Base for debug printing
A = 0                      # register for source address or scratch
B = 0                      # register for destination addr or scratch
T = 0                      # Top of data stack
S = 0                      # Second on data stack
R = 0                      # top of Return stack
PC = ag.Heap               # Program Counter
DSDEEP = 0                 # Data Stack Depth (count include T and S)
RSDEEP = 0                 # Return Stack Depth (count inlcudes R)
DSTACK = [0] * 16          # Data Stack
RSTACK = [0] * 16          # Return Stack
RAM = bytearray(MemMax+1)  # Random Access Memory
INBUF = b''                # Input buffer
OUTBUF = b''               # Output buffer
ECHO = False               # Echo depends on tty vs pip, etc.
HALTED = False             # Flag to track halt (used for `bye`)
STDOUT_IRQ = None          # IRQ line (callback) for buffering stdout
IOLOAD_DEPTH = 0           # Nesting level for io_load_file()
IOLOAD_FAIL = False        # Flag indicating an error during io_load_file()
# Debug tracing on/off
DBG_TRACE_ENABLE = False   # Debug trace is noisy, so default=off
# Debug symbols
DBG_ADDRS = []
DBG_NAMES = []
#
# Jump Table for instruction decoder. Table is populated towards end of file
# once all the necessary functions have been defined.
JUMP_TABLE: Dict[int, Callable] = {}
# Compile filename allow-list regular expressions for IOLOAD
ioload_allow_re = []
for line in IOLOAD_ALLOW_RE_LIST.strip().split("\n"):
  ioload_allow_re.append(re.compile(line))
# Compile filename allow-list regular expressions for IOSAVE
iosave_allow_re = []
for line in IOSAVE_ALLOW_RE_LIST.strip().split("\n"):
  iosave_allow_re.append(re.compile(line))


def reset_state(echo=False):
  """Initialize virtual CPU, RAM, and peripherals"""
  global ERR, BASE, A, B, T, S, R, PC, DSDEEP, RSDEEP, DSTACK, RSTACK, RAM
  global INBUF, OUTBUF, ECHO, HALTED, STDOUT_IRQ, IOLOAD_DEPTH
  global DBG_TRACE_ENABLE
  ERR = 0                    # Error register (don't confuse with ERR opcode!)
  BASE = 10                  # number Base for debug printing
  A = 0                      # register for source address or scratch
  B = 0                      # register for destination addr or scratch
  T = 0                      # Top of data stack
  S = 0                      # Second on data stack
  R = 0                      # top of Return stack
  PC = ag.Heap               # Program Counter
  DSDEEP = 0                 # Data Stack Depth (count include T and S)
  RSDEEP = 0                 # Return Stack Depth (count inlcudes R)
  DSTACK = [0] * 16          # Data Stack
  RSTACK = [0] * 16          # Return Stack
  RAM = bytearray(MemMax+1)  # Random Access Memory
  INBUF = b''                # Input buffer
  OUTBUF = b''               # Output buffer
  ECHO = echo                # Echo depends on tty vs pip, etc.
  HALTED = False             # Flag to track halt (used for `bye`)
  STDOUT_IRQ = None          # IRQ line (callback) for buffering stdout
  IOLOAD_DEPTH = 0           # Nesting level for io_load_file()
  IOLOAD_FAIL = False        # Flag indicating an error during io_load_file()
  # Debug tracing on/off
  DBG_TRACE_ENABLE = False   # Debug trace is noisy, so default=off

def dbg_add_symbol(addr, name):
  """Add a debug symbol entry to the symbol table"""
  DBG_ADDRS.append(int(addr))
  DBG_NAMES.append(name)

def dbg_addr_for(addr):
  """Given a name, return the address of the symbol it belongs to"""
  for (i, s_name) in enumerate(DBG_NAMES):
    if name == s_name:
      return dbg_addr[i]
  return 0

def dbg_name_for(addr):
  """Given an address, return the name of the symbol it belongs to"""
  for (i, s_addr) in enumerate(DBG_ADDRS):
    if addr >= s_addr:
      return DBG_NAMES[i]
  return '<???>'

def set_stdout_irq(stdout_irq_fn: Callable[[], None]):
  """Set callback function to raise interrupt line for available output.
  This is a little weird because of my desire to avoid building a bunch of
  `async` and `await` stuff into the VM class. This arrangement allows for
  an async function to provide the VM with input and ask it for pending
  output after the STDOUT_IRQ signal is sent.
  """
  global STDOUT_IRQ
  STDOUT_IRQ = stdout_irq_fn

def mkb_print(*args, **kwargs):
  """Send output to stdout (terminal mode) or buffer it (irq mode) """
  global OUTBUF
  if STDOUT_IRQ is None:
    # In termio mode send output to stdout
    print(OUTBUF.decode('utf8'), end='')
    OUTBUF = b''
    print(*args, **kwargs)
    sys.stdout.flush()
  else:
    # When using STDOUT_IRQ, buffer the output and raise the IRQ line
    end = kwargs.get('end', "\n").encode('utf8')
    new_stuff = (" ".join(args)).encode('utf8')
    new_stuff += end
    OUTBUF += new_stuff
    STDOUT_IRQ()

def drain_stdout():
  """Return contents of OUTBUF and clear it"""
  global OUTBUF
  outbuf_ = OUTBUF.decode('utf8')
  OUTBUF = b''
  return outbuf_

def _load_rom(code):
  """Load byte array of rom image into memory"""
  n = len(code)
  if n > (ag.HeapRes - ag.Heap):
    error(ERR_BOOT_OVERFLOW)
    return
  RAM[ag.Heap:ag.Heap+n] = code[0:]

def _warm_boot(code, max_cycles=1):
  """Load a byte array of machine code into memory, then run it."""
  global ERR, PC
  ERR = 0
  _load_rom(code)
  PC = ag.Heap       # hardcode boot vector to start of heap (for now)
  _step(max_cycles)

def irq_rx(line):
  """Interrupt request for receiving a line of input"""
  global INBUF, PC
  # First, copy the input line to the input buffer as UTF-8
  INBUF += line.encode('utf8')
  # Be sure the line ends with LF (some input sources do, some don't)
  if (len(line) == 0) or (line[-1] != "\n"):
    INBUF += b'\x0a'
  # When stdin is a pipe, echo input to stdout (minus its trailing newline)
  if (not STDOUT_IRQ) and ECHO:
    print(line[:-1], end='')
  # Next, attempt to jump to interrupt request vector for received input
  _push(IRQRX)
  load_halfword()
  irq_vector = T
  drop()
  # allow for the possibility that the interrupt vector is not set
  if irq_vector != 0:
    PC = irq_vector
    max_cycles = 65335
    _step(max_cycles)
  else:
    if DEBUG:
      print("<< IRQRX is 0 >>>")

def irq_err(err_):
  """Handle error interrupt by jumping via error handler vector (IRQERR)"""
  global PC, IOLOAD_FAIL
  # Clear the VM's stacks, input buffer, and error code
  reset()
  _push(err_)
  # If this error happened while a file load was in progress, set a flag to
  # prevent spurious OK prompts as the io_load_file() calls unwind
  if IOLOAD_DEPTH > 0:
    IOLOAD_FAIL = True
  # Set program counter to jump to the error handler vector
  addr = (RAM[IRQERR+1] << 8) | RAM[IRQERR]  # LE halfword
  if addr > ag.HeapMax:
    mkb_print("  --\nError... IRQERR didn't work... emergency reboot")
    PC = 0
  else:
    PC = addr

def _step(max_cycles):
  """Step the virtual CPU clock to run up to max_cycles instructions.
  This is the non-debug version with a shorter inner loop vs _step_debug()."""
  global PC, RSDEEP
  if DEBUG:
    _step_debug(max_cycles)
    return
  for _ in range(max_cycles):
    pc = PC
    if pc < 0 or pc >= ag.HeapMax:
      # Stop if program counter somehow got set to address not in dictionary
      irq_err(ERR_BAD_PC_ADDR)
      continue
    PC += 1
    op = RAM[pc]
    if RSDEEP == 0 and op == ag.RET:
      return
    if op in JUMP_TABLE:
      (JUMP_TABLE[op])()
    else:
      irq_err(ERR_BAD_INSTRUCTION)
      continue
  # Making it here means the VM interrupted the code because it ran too long.
  # This probably means things have gone haywire, and potentially the memory
  # state is corrupted. But, try to make the best of it. So, since the kernel
  # didn't get a chance to print its own prompt, set the error code and
  # print an error prompt. Also, clear the return stack
  irq_err(ERR_MAX_CYCLES)
  _step(65535)

def _step_debug(max_cycles):
  """Step the virtual CPU clock up to max_cycles, with DEBUG tracing.
  It's worth giving this its own function with some code duplication against
  _step() because the inner loop of _step() runs so frequently. Pulling this
  extra stuff into its own function, rather than putting it in an if-iv block
  of _step(), makes a noticable profiling improvement.
  """
  global PC, RSDEEP
  for _ in range(max_cycles):
    pc = PC
    if pc < 0 or pc >= ag.HeapMax:
      # Stop if program counter somehow got set to address not in dictionary
      irq_err(ERR_BAD_PC_ADDR)
      continue
    PC += 1
    op = RAM[pc]
    if DBG_TRACE_ENABLE:
      name = [k for (k,v) in OPCODES.items() if v == op]
      if len(name) == 1:
        name = name[0]
      else:
        name = f"{name}"
      print(f"<<{pc:04x}:{op:2}: {dbg_name_for(pc):9}:{name:6}", end='')
      _log_ds()
      print(f" ERR:{ERR}>>")
    if RSDEEP == 0 and op == ag.RET:
      return
    if op in JUMP_TABLE:
      (JUMP_TABLE[op])()
    else:
      print("\n========================")
      print("DStack: ", end='')
      io_data_stack()
      print("\nRStack: ", end='')
      io_return_stack_hex()
      print(f"\nPC: 0x{PC:04x} = {PC}")
      raise Exception("bad instruction", op)
  # Making it here means the VM interrupted the code because it ran too long,
  # or one of the instructions triggered an error. This probably means things
  # have gone haywire, with the stacks and program counter corrupted. But, try
  # to make the best of it. Since the kernel didn't get a chance to print its
  # own prompt, clear the return stack, set the right error code, and jump to
  # the kernel's error prompt vector.
  irq_err(ERR_MAX_CYCLES)
  _step(65535)

def _op_st(fn):
  """Apply operation λ(S,T), storing the result in S and dropping T"""
  global S, T, DSDEEP
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  n = fn(S, T) & 0xffffffff
  T = (n & 0x7fffffff) - (n & 0x80000000)  # sign extend i32->whatever
  # The rest of this function is an unroll of a call to drop() with a slight
  # optimization to avoid copying S to T. According to cProfile, _op_st() is
  # one of the most frequently called functions in the VM. Avoiding the extra
  # function call here speeds it up measurably.
  if DSDEEP > 2:
    third = DSDEEP - 3
    S = DSTACK[third]
  DSDEEP -= 1

def _op_t(fn):
  """Apply operation λ(T), storing the result in T"""
  global T
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  n = fn(T) & 0xffffffff
  T = (n & 0x7fffffff) - (n & 0x80000000)  # sign extend i32->whatever

def _push(n):
  """Push n onto the data stack as a 32-bit signed integer"""
  global S, T, DSDEEP
  if DSDEEP > 17:
    irq_err(ERR_D_OVER)   # Set error code
    return
  if DSDEEP > 1:
    third = DSDEEP - 2
    DSTACK[third] = S
  S = T
  T = n
  DSDEEP += 1

def nop():
  """Do nothing, but consume a little time for the non-doing"""
  pass

def and_():
  """Store bitwise AND of S with T into S, then drop T"""
  _op_st(lambda s, t: s & t)

def load_byte():
  """Load a uint8 (1 byte) from memory address T, saving result in T"""
  global T
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  addr = T & 0xffff
  T = RAM[addr]

def load_byte_a():
  """Load byte from memory using address in register A"""
  addr = A & 0xffff
  x = RAM[addr]
  _push(x)

def load_byte_b():
  """Load byte from memory using address in register B"""
  addr = B & 0xffff
  x = RAM[addr]
  _push(x)

def load_byte_a_increment():
  """Load byte from memory using address in register A, then increment A"""
  global A
  addr = A & 0xffff
  x = RAM[addr]
  A += 1
  _push(x)

def load_byte_b_increment():
  """Load byte from memory using address in register B, then increment B"""
  global B
  addr = B & 0xffff
  x = RAM[addr]
  B += 1
  _push(x)

def store_byte_b_increment():
  """Store low byte of T byte to address in register B, then increment B"""
  global B
  addr = B & 0xffff
  B += 1
  RAM[addr] = T & 0xff
  drop()

def store_byte():
  """Store low byte of S (uint8) at memory address T"""
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  addr = T & 0xffff
  RAM[addr] = S & 0xff
  drop()
  drop()

def call():
  """Call to subroutine at address T, pushing old PC to return stack"""
  global R, RSDEEP, PC
  if RSDEEP > 16:
    reset()
    irq_err(ERR_R_OVER)
    return
  # push the current Program Counter (PC) to return stack
  if RSDEEP > 0:
    rSecond = RSDEEP - 1
    RSTACK[rSecond] = R
  R = PC
  RSDEEP += 1
  # set Program Counter to the new address
  PC = T & 0xffff
  drop()

def jump_and_link():
  """Jump to subroutine after pushing old value of PC to return stack.
  The jump address is PC-relative to allow for relocatable object code.
  """
  global R, RSDEEP, PC
  if RSDEEP > 16:
    reset()
    irq_err(ERR_R_OVER)
    return
  # read a 16-bit signed offset (relative to PC) from instruction stream
  pc = PC
  n = (RAM[pc+1] << 8) + RAM[pc]  # decode little-endian halfword
  n = (n & 0x7fff) - (n & 0x8000)           # sign extend it
  # push the current Program Counter (PC) to return stack
  if RSDEEP > 0:
    rSecond = RSDEEP - 1
    RSTACK[rSecond] = R
  R = pc + 2
  RSDEEP += 1
  # add offset to program counter to compute destination address.
  # the 0xffff mask lets you do stuff like (5-100) & 0xffff = 65441 so a
  # a signed 16-bit pc-relative offset can address the full memory range
  PC = (pc + n) & 0xffff

def drop():
  """Drop T, the top item of the data stack"""
  global T, S, DSDEEP
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  T = S
  if DSDEEP > 2:
    third = DSDEEP - 3
    S = DSTACK[third]
  DSDEEP -= 1

def dup():
  """Push a copy of T"""
  _push(T)

def equal():
  """Evaluate S == T (true:-1, false:0), store result in S, drop T"""
  global T, S, DSDEEP
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  T = -1 if (S == T) else 0
  # The rest of this function is an unroll of a call to drop() with a slight
  # optimization to avoid copying S to T. According to cProfile, equal() is
  # one of the most frequently called functions in the VM. Avoiding the extra
  # function call here speeds it up measurably.
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  if DSDEEP > 2:
    third = DSDEEP - 3
    S = DSTACK[third]
  DSDEEP -= 1

def load_word():
  """Load a signed int32 (word = 4 bytes) from memory address T, into T"""
  global T
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  addr = T & 0xffff
  if addr > MemMax-3:
    irq_err(ERR_BAD_ADDRESS)
    return
  T = int.from_bytes(RAM[addr:addr+4], 'little', signed=True)

def greater_than():
  """Evaluate S > T (true:-1, false:0), store result in S, drop T"""
  _op_st(lambda s, t: -1 if s > t else 0)

def invert():
  """Invert the bits of T (ones' complement negation)"""
  _op_t(lambda t: ~ t)

def jump():
  """Jump to subroutine at address read from instruction stream.
  The jump address is PC-relative to allow for relocatable object code.
  """
  global PC
  # read a 16-bit PC-relative address offset from the instruction stream
  pc = PC
  n = (RAM[pc+1] << 8 ) | RAM[pc]  # LE halfword
  n = (n & 0x7fff) - (n & 0x8000)           # sign extend it
  # add offset to program counter to compute destination address.
  # the 0xffff mask lets you do stuff like (5-100) & 0xffff = 65441 so a
  # a signed 16-bit pc-relative offset can address the full memory range
  PC = (PC + n) & 0xffff

def branch_zero():
  """Branch to PC-relative address if T == 0, drop T.
  The branch address is PC-relative to allow for relocatable object code.
  """
  global PC, T, S, DSDEEP
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  pc = PC
  if T == 0:
    # Branch forward past conditional block: Add address literal from
    # instruction stream to PC. Maximum branch distance is +255.
    PC = pc + RAM[pc]
  else:
    # Enter conditional block: Advance PC past address literal
    PC = pc + 1
  # The rest of this function is an unroll of a call to drop(). According to
  # cProfile, branch_zero() is one of the most frequently called functions in
  # the VM. Avoiding the extra function call here speeds it up measurably.
  T = S
  if DSDEEP > 2:
    third = DSDEEP - 3
    S = DSTACK[third]
  DSDEEP -= 1

def branch_for_loop():
  """Decrement R and branch to start of for-loop if R >= 0.
  The branch address is PC-relative to allow for relocatable object code.
  """
  global R, PC
  if RSDEEP < 1:
    irq_err(ERR_R_UNDER)
    return
  R -= 1
  pc = PC
  if R >= 0:
    # Keep looping: Branch backwards by subtracting byte literal from PC
    # Maximum branch distance is -255
    PC -= RAM[pc]
  else:
    # End of loop: Advance PC past address literal, drop R
    PC += 1
    r_drop()

def less_than():
  """Evaluate S < T (true:-1, false:0), store result in S, drop T"""
  _op_st(lambda s, t: -1 if s < t else 0)

def u16_literal():
  """Read uint16 halfword (2 bytes) literal, zero-extend it, push as T"""
  global PC, S, T, DSDEEP
  pc = PC
  n = (RAM[pc+1] << 8) | RAM[pc]  # LE halfword
  PC += 2
  # The rest of this function is an unroll of _push(). The point is to save
  # the overhead of a function call since U16 gets called very frequently.
  if DSDEEP > 17:
    irq_err(ERR_D_OVER)   # Set error code
    return
  if DSDEEP > 1:
    third = DSDEEP - 2
    DSTACK[third] = S
  S = T
  T = n
  DSDEEP += 1

def i32_literal():
  """Read int32 word (4 bytes) signed literal, push as T"""
  global PC
  pc = PC
  n = int.from_bytes(RAM[pc:pc+4], 'little', signed=True)
  PC += 4
  _push(n)

def u8_literal():
  """Read uint8 byte literal, zero-extend it, push as T"""
  global PC, S, T, DSDEEP
  pc = PC
  n = RAM[pc]
  PC = pc + 1
  # The rest of this function is an unroll of _push(). The point is to save
  # the overhead of a function call since U8 gets called very frequently.
  if DSDEEP > 17:
    irq_err(ERR_D_OVER)   # Set error code
    return
  if DSDEEP > 1:
    third = DSDEEP - 2
    DSTACK[third] = S
  S = T
  T = n
  DSDEEP += 1

def subtract():
  """Subtract T from S, store result in S, drop T"""
  _op_st(lambda s, t: s - t)

def decrement():
  """Subtract 1 from T"""
  _op_t(lambda t: t - 1)

def multiply():
  """Multiply S by T, store result in S, drop T"""
  _op_st(lambda s, t: s * t)

def divide():
  """Divide S by T (integer division), store quotient in S, drop T"""
  _op_st(lambda s, t: s // t)

def modulo():
  """Divide S by T (integer division), store remainder in S, drop T"""
  _op_st(lambda s, t: s % t)

def not_equal():
  """Evaluate S != T (true:-1, false:0), store result in S, drop T"""
  global T, S, DSDEEP
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  T = -1 if (S != T) else 0
  # The rest of this function is an optimized unroll of a call to drop()
  if DSDEEP > 2:
    third = DSDEEP - 3
    S = DSTACK[third]
  DSDEEP -= 1

def or_():
  """Store bitwise OR of S with T into S, then drop T"""
  _op_st(lambda s, t: s | t)

def over():
  """Push a copy of S"""
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  _push(S)

def add():
  """Add T to S, store result in S, drop T"""
  _op_st(lambda s, t: s + t)

def increment():
  """Add 1 to T"""
  _op_t(lambda t: t + 1)

def a_increment():
  """Add 1 to register A"""
  global A
  A += 1

def a_decrement():
  """Subtract 1 from register A"""
  global A
  A -= 1

def b_increment():
  """Add 1 to register B"""
  global B
  B += 1

def b_decrement():
  """Subtract 1 from register B"""
  global B
  B -= 1

def reset():
  """Reset the data stack, return stack, error code, and input buffer"""
  global DSDEEP, RSDEEP, ERR, INBUF
  DSDEEP = 0
  RSDEEP = 0
  ERR = 0
  INBUF = b''

def return_():
  """Return from subroutine, taking address from return stack"""
  global PC, R, RSDEEP
  if RSDEEP < 1:
    irq_err(ERR_R_UNDER)
    return
  # Set program counter from top of return stack
  PC = R
  # Drop top of return stack
  if RSDEEP > 1:
    rSecond = RSDEEP - 2
    R = RSTACK[rSecond]
  RSDEEP -= 1

def halt():
  """Set the halt flag to stop further instructions (used for `bye`)"""
  global HALTED
  HALTED = True

def io_data_stack():
  _log_ds(base=10)

def io_data_stack_hex():
  _log_ds(base=16)

def io_return_stack_hex():
  _log_rs(base=16)

def r_drop():
  """Drop R in the manner needed when exiting from a counted loop"""
  global R, RSDEEP
  if RSDEEP < 1:
    irq_err(ERR_R_UNDER)
    return
  if RSDEEP > 1:
    rSecond = RSDEEP - 2
    R = RSTACK[rSecond]
  RSDEEP -= 1

def shift_left_logical():
  """Shift S left by T, store result in S, drop T"""
  _op_st(lambda s, t: s << t)

def shift_right_arithmetic():
  """Signed (arithmetic) shift S right by T, store result in S, drop T"""
  # Python right shift is always an arithmetic (signed) shift
  _op_st(lambda s, t: s >> t)

def shift_right_logical():
  """Unsigned (logic) shift S right by T, store result in S, drop T"""
  # Mask first because Python right shift is always signed
  _op_st(lambda s, t: (s & 0xffffffff) >> t)

def store_word():
  """Store word (4 bytes) from S as signed int32 at memory address T"""
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  addr = T & 0xffff
  x = int.to_bytes(c_int32(S).value, 4, 'little', signed=True)
  if addr > MemMax-3:
    irq_err(ERR_BAD_ADDRESS)
  else:
    RAM[addr:addr+4] = x
  drop()
  drop()

def swap():
  """Swap S with T"""
  global T, S
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  tmp = T
  T = S
  S = tmp

def move_t_to_r():
  """Move top of data stack (T) to top of return stack (R)"""
  global R, RSDEEP
  if RSDEEP > 16:
    irq_err(ERR_R_OVER)
    return
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  if RSDEEP > 0:
    rSecond = RSDEEP - 1
    RSTACK[rSecond] = R
  R = T
  RSDEEP += 1
  drop()

def move_t_to_a():
  """Move top of data stack (T) to register A"""
  global A
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  A = T
  drop()

def move_t_to_b():
  """Move top of data stack (T) to register B"""
  global B
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  B = T
  drop()

def move_t_to_err():
  """Move top of data stack (T) to ERR register (raise an error)"""
  global ERR
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  ERR = T
  drop()
  irq_err(ERR)

def load_halfword():
  """Load halfword (2 bytes, zero-extended) from memory address T, into T"""
  global T
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  addr = T & 0xffff
  if addr > MemMax-1:
    irq_err(ERR_BAD_ADDRESS)
    return
  T = (RAM[addr+1] << 8) | RAM[addr]  # LE halfword

def store_halfword():
  """Store low 2 bytes from S (uint16) at memory address T"""
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  addr = T & 0xffff
  if addr > MemMax-1:
    irq_err(ERR_BAD_ADDRESS)
  else:
    RAM[addr] = S & 0xff           # LE halfword low byte
    RAM[addr+1] = (S >> 8) & 0xff  # LE halfword high byte
  drop()
  drop()

def xor():
  """Store bitwise XOR of S with T into S, then drop T"""
  _op_st(lambda s, t: s ^ t)

def zero_equal():
  """Evaluate 0 == T (true:-1, false:0), store result in T"""
  global T
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  T = -1 if (T == 0) else 0

def true_():
  """Push -1 (true) to data stack"""
  _push(-1)

def false_():
  """Push 0 (false) to data stack"""
  _push(0)

def _log_ds(base=10):
  """Log (debug print) the data stack in the manner of .S"""
  buf = " "
  if DSDEEP > 2:
    for i in range(DSDEEP-2):
      n = DSTACK[i]
      if base == 16:
        buf += f" {n&0xffffffff:x}"
      else:
        buf += f" {n}"
  if DSDEEP > 1:
    if base == 16:
      buf += f" {S&0xffffffff:x}"
    else:
      buf += f" {S}"
  if DSDEEP > 0:
    if base == 16:
      buf += f" {T&0xffffffff:x}"
    else:
      buf += f" {T}"
  else:
    buf += " Stack is empty"
  mkb_print(buf, end='')

def _log_rs(base=10):
  """Log (debug print) the return stack in the manner of .S"""
  buf = " "
  if RSDEEP > 1:
    for i in range(RSDEEP-1):
      n = RSTACK[i]
      if base == 16:
        buf += f" {n&0xffffffff:x}"
      else:
        buf += f" {n}"
  if RSDEEP > 0:
    if base == 16:
      buf += f" {R&0xffffffff:x}"
    else:
      buf += f" {R}"
  else:
    buf += " R-Stack is empty"
  mkb_print(buf, end='')

def r_():
  """Push a copy of top of Return stack (R) to the data stack"""
  _push(R)

def pc_():
  """Push a copy of the Program Counter (PC) to the data stack"""
  _push(PC)

def a_():
  """Push a copy of register A to the data stack"""
  _push(A)

def b_():
  """Push a copy of register B to the data stack"""
  _push(B)

def io_key():
  """Push the next byte from Standard Input to the data stack.

  Results (stack effects):
  - Got an input byte, push 2 items: {S: byte, T: -1 (true)}
  - End of file, push 1 item:           {T: 0 (false)}
  """
  global INBUF
  if len(INBUF) > 0:
    _push(INBUF[0])
    _push(-1)
    INBUF = INBUF[1:]
  else:
    _push(0)

def io_emit():
  """Buffer the low byte of T for stdout.

  This is meant to allow for utf-8 sequences to be emitted 1 byte at a time
  without getting into fights with Python over its expectations about string
  encoding. This output method is most suitable for tests, demo code, and
  prototypes, where simplicity of the code is more important than its
  efficiency. Using this method to print long strings would be inefficient.
  """
  global OUTBUF
  new_stuff = int.to_bytes(T & 0xff, 1, 'little')
  OUTBUF += new_stuff
  if T == 10:
    mkb_print(end='')
  drop()

def io_dot():
  """Print T to standard output, then drop T"""
  global OUTBUF
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  new_stuff = f" {T}".encode('utf8')
  OUTBUF += new_stuff
  drop()

def io_dump():
  """Hexdump S bytes of memory starting from address T, then drop S and T"""
  if DSDEEP < 2:
    irq_err(ERR_D_UNDER)
    return
  bad_start_addr = (T < 0) or (T > MemMax)
  bad_byte_count = (S < 0) or (T + S > MemMax)
  if bad_start_addr or bad_byte_count:
    irq_err(ERR_BAD_ADDRESS)
    return
  col = 0
  start = T
  end = T + S
  drop()
  drop()
  left = ""
  right = ""
  dirty = 0
  for (i, n) in enumerate(RAM[start:end]):
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
      mkb_print(f"{left}  {right}")
      dirty = False
    col = (col + 1) & 15
  if dirty:
    # if number of bytes requested was not an even multiple of 16, then
    # print what's left of the last row
    mkb_print(f"{left:41}  {right}")

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
def _load_string(addr):
  """Load a Markab string from RAM[addr] and return it as a python string"""
  addr = T & 0xffff
  count = RAM[addr]
  if addr + count < len(RAM):
    str_ = (RAM[addr+1:addr+count+1]).decode('utf8')
    return str_
  else:
    if DEBUG:
      print("<<< _load_string(): bad address >>>")
    return ""

# ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
# If you are reviewing for possible security issues, pay attention here
def _normalize_filepath(filepath):
  path = os.path.normcase(os.path.abspath(filepath))
  if DEBUG:
    print(f"<<< _normalize_filepath() -> '{path}' >>>")
  return path

# ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
# If you are reviewing for possible security issues, pay attention here
def _is_file_in_cwd(filepath):
  """Return whether file path is in the current working directory"""
  cwd = _normalize_filepath(os.getcwd())
  abs_path = _normalize_filepath(filepath)
  if abs_path.startswith(cwd):
    return True
  else:
    return False

# ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
# If you are reviewing for possible security issues, pay attention here
def io_load_file():
  """Load and interpret file, taking file path from string pointer in T.
  File path must pass two tests:
  1. Match one of the allow-list regular expressions for IOLOAD
  2. Be located in the current working directory (including subdirectories)
  """
  global ECHO, IOLOAD_DEPTH, PC, INBUF, IOLOAD_FAIL
  if DSDEEP < 1:
    irq_err(ERR_D_UNDER)
    return
  if IOLOAD_DEPTH > 1:
    irq_err(ERR_IOLOAD_DEPTH)
    return
  filepath = _load_string(T)
  drop()
  # Check filename against the allow list
  re_match = any([a.match(filepath) for a in ioload_allow_re])
  cwd_match = _is_file_in_cwd(filepath)
  if DEBUG:
    print(f"<<< IOLOAD: re match '{filepath}' ? --> {re_match} >>>")
    print(f"<<< IOLOAD: cwd match '{filepath}' ? --> {cwd_match} >>>")
  if (not re_match) or (not cwd_match):
    irq_err(ERR_FILE_PERMS)
    return
  try:
    # Save VM state
    old_inbuf = INBUF
    old_pc = PC
    old_echo = ECHO
    # Read and interpret the file
    ECHO = False
    cascade_errors = True
    with open(filepath) as f:
      IOLOAD_DEPTH += 1
      for (n, line) in enumerate(f):
        irq_rx(line)
        if IOLOAD_FAIL:
          line = line.split("\n")[0]
          mkb_print(f"{filepath}:{n+1}: {line}", end='')
          irq_err(ERR_IOLOAD_FAIL)
          break
    # Restore old state
    IOLOAD_DEPTH -= 1
    PC = old_pc
    ECHO = old_echo
    if (ERR == 0) and (not IOLOAD_FAIL):
      INBUF = old_inbuf
      # This is the happy path to the land of "  OK", so no cascading errors
      cascade_errors = False
    if IOLOAD_DEPTH == 0:
      IOLOAD_FAIL = 0
  except FileNotFoundError:
    irq_err(ERR_FILE_NOT_FOUND)
    return
  # CAUTION! Checking IOLOAD_FAIL here would be wrong.
  if cascade_errors:
    irq_err(ERR_IOLOAD_FAIL)

# ⚠️  DANGER! DANGER! DANGER! LOTS OF DANGER! ⚠️
# If you are reviewing for possible security issues, pay attention here
def io_save_file():
  """Save memory to file, T: filename, S: source address, 3rd: byte count.
  File path must pass two tests:
  1. Match one of the allow-list regular expressions for IOSAVE
  2. Be located in the current working directory (including subdirectories)
  """
  # //////////////////////////////////////////////////////////////////
  print("//////// TODO FINISH IMPLEMENTING io_save_file() ///////////")
  # //////////////////////////////////////////////////////////////////
  if DSDEEP < 3:
    irq_err(ERR_D_UNDER)
    return
  # Pop arguments off stack before doing any error checks
  filepath = _load_string(T)
  drop()
  src_addr = T & 0xffff
  drop()
  count = T & 0xffff
  drop()
  # Check the filename against the allow list
  re_match = any([a.match(filepath) for a in iosave_allow_re])
  cwd_match = _is_file_in_cwd(filepath)
  if DEBUG:
    print(f"<<< IOSAVE: re match '{filepath}' ? --> {re_match} >>>")
    print(f"<<< IOSAVE: cwd match '{filepath}' ? --> {cwd_match} >>>")
  if (not re_match) or (not cwd_match):
    irq_err(ERR_FILE_PERMS)
    return
  # Check if source address and byte count are reasonable
  if src_addr + count > len(RAM):
    irq_err(ERR_BAD_ADDRESS)
    return
  #///////////////////////////////////////////
  # TODO: make this part actually save a file
  #///////////////////////////////////////////

# =======================================================================
# === END OF DANGEROUS FILE IO STUFF ====================================
# =======================================================================

def trace_on():
  """Enable debug tracing (also see DEBUG global var)"""
  global DBG_TRACE_ENABLE
  DBG_TRACE_ENABLE = True

def trace_off():
  """Disable debug tracing (also see DEBUG global var)"""
  global DBG_TRACE_ENABLE
  DBG_TRACE_ENABLE = False

def _ok_or_err():
  """Print the OK or ERR line-end status message and clear any errors"""
  global ERR
  if ERR != 0:
    mkb_print(f"  ERR {ERR}")
    ERR = 0
  else:
    mkb_print("  OK")

# ========================================================================
# === END OF VM OPCODE IMPLEMENTATION FUNCTIONS ==========================
# ========================================================================

# Populate the jump table now that all the necessary functions are defined
JUMP_TABLE[ag.NOP   ] = nop
JUMP_TABLE[ag.ADD   ] = add
JUMP_TABLE[ag.SUB   ] = subtract
JUMP_TABLE[ag.INC   ] = increment
JUMP_TABLE[ag.DEC   ] = decrement
JUMP_TABLE[ag.MUL   ] = multiply
JUMP_TABLE[ag.DIV   ] = divide
JUMP_TABLE[ag.MOD   ] = modulo
JUMP_TABLE[ag.AND   ] = and_
JUMP_TABLE[ag.INV   ] = invert
JUMP_TABLE[ag.OR    ] = or_
JUMP_TABLE[ag.XOR   ] = xor
JUMP_TABLE[ag.SLL   ] = shift_left_logical
JUMP_TABLE[ag.SRL   ] = shift_right_logical     # zero extend
JUMP_TABLE[ag.SRA   ] = shift_right_arithmetic  # sign extend
JUMP_TABLE[ag.EQ    ] = equal
JUMP_TABLE[ag.GT    ] = greater_than
JUMP_TABLE[ag.LT    ] = less_than
JUMP_TABLE[ag.NE    ] = not_equal
JUMP_TABLE[ag.ZE    ] = zero_equal
JUMP_TABLE[ag.TRUE  ] = true_
JUMP_TABLE[ag.FALSE ] = false_
JUMP_TABLE[ag.JMP   ] = jump
JUMP_TABLE[ag.JAL   ] = jump_and_link
JUMP_TABLE[ag.CALL  ] = call
JUMP_TABLE[ag.RET   ] = return_
JUMP_TABLE[ag.HALT  ] = halt
JUMP_TABLE[ag.BZ    ] = branch_zero
JUMP_TABLE[ag.BFOR  ] = branch_for_loop
JUMP_TABLE[ag.MTR   ] = move_t_to_r
JUMP_TABLE[ag.RDROP ] = r_drop
JUMP_TABLE[ag.R     ] = r_
JUMP_TABLE[ag.PC    ] = pc_
JUMP_TABLE[ag.MTE   ] = move_t_to_err
JUMP_TABLE[ag.DROP  ] = drop
JUMP_TABLE[ag.DUP   ] = dup
JUMP_TABLE[ag.OVER  ] = over
JUMP_TABLE[ag.SWAP  ] = swap
JUMP_TABLE[ag.U8    ] = u8_literal
JUMP_TABLE[ag.U16   ] = u16_literal
JUMP_TABLE[ag.I32   ] = i32_literal
JUMP_TABLE[ag.LB    ] = load_byte
JUMP_TABLE[ag.SB    ] = store_byte
JUMP_TABLE[ag.LH    ] = load_halfword
JUMP_TABLE[ag.SH    ] = store_halfword
JUMP_TABLE[ag.LW    ] = load_word
JUMP_TABLE[ag.SW    ] = store_word
JUMP_TABLE[ag.RESET ] = reset
JUMP_TABLE[ag.IOD   ] = io_data_stack
JUMP_TABLE[ag.IODH  ] = io_data_stack_hex
JUMP_TABLE[ag.IORH  ] = io_return_stack_hex
JUMP_TABLE[ag.IOKEY ] = io_key
JUMP_TABLE[ag.IOEMIT] = io_emit
JUMP_TABLE[ag.IODOT ] = io_dot
JUMP_TABLE[ag.IODUMP] = io_dump
JUMP_TABLE[ag.IOLOAD] = io_load_file
JUMP_TABLE[ag.IOSAVE] = io_save_file
JUMP_TABLE[ag.TRON  ] = trace_on
JUMP_TABLE[ag.TROFF ] = trace_off
JUMP_TABLE[ag.MTA   ] = move_t_to_a
JUMP_TABLE[ag.LBA   ] = load_byte_a
JUMP_TABLE[ag.LBAI  ] = load_byte_a_increment
JUMP_TABLE[ag.AINC  ] = a_increment
JUMP_TABLE[ag.ADEC  ] = a_decrement
JUMP_TABLE[ag.A     ] = a_
JUMP_TABLE[ag.MTB   ] = move_t_to_b
JUMP_TABLE[ag.LBB   ] = load_byte_b
JUMP_TABLE[ag.LBBI  ] = load_byte_b_increment
JUMP_TABLE[ag.SBBI  ] = store_byte_b_increment
JUMP_TABLE[ag.BINC  ] = b_increment
JUMP_TABLE[ag.BDEC  ] = b_decrement
JUMP_TABLE[ag.B     ] = b_

# ========================================================================
# === END OF VM IMPLEMENTATION ===========================================
# ========================================================================


def termio_boot(rom_bytes, max_cycles):
  """VM bootload and input loop for terminal mode"""
  _warm_boot(rom_bytes, max_cycles)
  _push(IRQRX)
  load_halfword()
  if T == 0:
    # if boot code did not set a receive IRQ vector, don't start input loop
    return
  drop()
  for line in sys.stdin:
    # Input comes from readline, which is line buffered, so this blocks the
    # thread until a full line of text is available. The VM is expected to
    # process the line of input and then return promptly.
    irq_rx(line)
    if HALTED:
      # this happens when `bye` signals it wants to exit
      break

def termio_main(rom_bytes, max_cycles):
  """Start the VM in terminal IO mode"""
  if PROFILE:
    # This will print a chart of function call counts and timings at exit
    cProfile.run('termio_boot(rom_bytes, 65535)', sort='cumulative')
  else:
    termio_boot(rom_bytes, max_cycles)
  if ECHO:
    # add a final newline if input is coming from pipe
    print()


"""
Load and boot the ROM file when VM is run as a module rather than imported
"""
if __name__ == '__main__':

  rom = ROM_FILE
  args = sys.argv[1:]

  # Check for a command line argument asking for a different rom.
  # For example: `./markab_vm.py hello.rom`
  if (len(args) > 0) and (args[-1].endswith(".rom")):
      rom = args[-1]

  # Make a VM instance
  reset_state(echo=(not sys.stdin.isatty()))

  # Attempt to load debug symbols (e.g. for kernel.rom, check kernel.symbols)
  sym_addrs = []
  sym_names = []
  if DEBUG:
    sym_file = rom[:-4] + ".symbols"
    try:
      with open(sym_file, 'r') as f:
        lines = f.read().strip().split("\n")
        lines = [L.split() for L in lines]
        lines.reverse()
        for (addr, name) in lines:
          dbg_add_symbol(addr, name)
    except:
      pass

  # Load the rom file
  rom_bytes = b''
  with open(rom, 'rb') as f:
    rom_bytes = f.read()

  # Boot the VM in terminal IO mode
  termio_main(rom_bytes, 65535)
