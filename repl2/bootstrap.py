#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab bootstrap compiler
#
from mkb_autogen import (
  JMP, JAL, RET, BZ, BFOR, MTR, U8, U16, I32, SH,
  T_VAR, T_CONST, T_OP, T_OBJ, T_IMM,
  Heap, HeapMax, CONTEXT, CURRENT, DP,
  OPCODES,
)
from markab_vm import VM

from os.path import basename, normpath


# This controls 16-byte alignment of dictionary entries: false is smaller
ALIGN16 = False  #True

SRC_IN = 'kernel.mkb'
ROM_OUT = 'kernel.rom'
SYM_OUT = 'kernel.symbols'
MODE_INT = 0
MODE_COM = 1


class Compiler:
  def __init__(self):
    """Initialize an Markab VM ROM instance for compiling into"""
    self.vm = VM()   # make a VM instance to use for compiling
    self.DP = Heap
    self.link = 0
    self.base = 10
    self.mode = MODE_INT
    self.last_call = 0
    self.nest_if = 0
    self.nest_for = 0
    self.name_set = {}
    self.link_set = {}
    self.append_byte(U16)       # compile initializer for CONTEXT
    self.init_context = self.DP
    self.append_halfword(0)
    self.append_byte(U16)
    self.append_halfword(CONTEXT)
    self.append_byte(SH)
    self.append_byte(U16)       # compile initializer for CURRENT
    self.init_current = self.DP
    self.append_halfword(0)
    self.append_byte(U16)
    self.append_halfword(CURRENT)
    self.append_byte(SH)
    self.append_byte(U16)       # compile initializer for DP
    self.init_dp = self.DP
    self.append_halfword(0)
    self.append_byte(U16)
    self.append_halfword(DP)
    self.append_byte(SH)
    self.append_byte(JMP)       # compile a boot jump to be patched later
    self.boot_vector = self.DP
    self.append_halfword(0)

  def store_byte(self):
    """Wrapper for vm.store_byte() to allow for easy debug logging"""
    self.vm.store_byte()

  def store_halfword(self):
    """Wrapper for vm.store_halfword() to allow for easy debug logging"""
    self.vm.store_halfword()

  def store_word(self):
    """Wrapper for vm.store_word() to allow for easy debug logging"""
    self.vm.store_word()

  def push(self, x):
    """Wrapper for vm._push() to allow for easy debug logging"""
    self.vm._push(x)

  def patch_boot_addr(self, addr):
    """Patch the target address for the boot jump at start of rom"""
    self.push(addr)
    self.push(self.boot_vector)
    self.store_halfword()

  def patch_context_current_dp(self):
    """Patch the initializer addresses at start of rom"""
    self.push(self.link)
    self.push(self.init_context)
    self.store_halfword()
    self.push(self.link)
    self.push(self.init_current)
    self.store_halfword()
    self.push(self.DP)
    self.push(self.init_dp)
    self.store_halfword()

  def append_byte(self, data):
    """Append 1 byte of data to the target rom dictionary"""
    self.push(data)
    self.push(self.DP)
    self.store_byte()
    self.DP += 1

  def append_halfword(self, data):
    """Append 2 bytes of data (16-bit halfword) to the target rom dictionary"""
    self.push(data)
    self.push(self.DP)
    self.store_halfword()
    self.DP += 2

  def append_word(self, data):
    """Append 4 bytes of data (32-bit word) to the target rom dictionary"""
    self.push(data)
    self.push(self.DP)
    self.store_word()
    self.DP += 4

  def create(self, name):
    """Start a named dictionary entry in the target rom"""
    offset = self.DP & 0xf
    if ALIGN16 and offset != 0:
      self.DP += 16 - offset       # align 16 for nicer hexdumps
    starting_dp = self.DP
    self.append_halfword(self.link)
    self.link = starting_dp
    data = name.encode('utf8')
    self.append_byte(len(data))
    for x in data:
      self.append_byte(x)
    if name == 'boot':
      # The name 'boot' triggers magic to set the boot jump target address
      self.patch_boot_addr(self.DP+1)
    self.name_set[name] = (None, self.link)
    self.link_set[self.link] = name

  def update_name_type(self, name, type_):
    """Update the type of a name in the name set"""
    (_, link) = self.name_set[name]
    self.name_set[name] = (type_, link)

  def var(self, name):
    """Add a dictionary entry for a named variable word to the target rom"""
    self.create(name)
    self.append_byte(T_VAR)
    self.append_word(0)
    self.update_name_type(name, T_VAR)

  def const(self, name):
    """Add a dictionary entry for a named constant word to the target rom"""
    if self.vm.DSDeep < 1:
      raise Exception("const: stack underflow")
    self.create(name)
    self.append_byte(T_CONST)
    value = self.vm.T           # take constant value from top of stack
    self.vm.drop()
    self.append_word(value)
    self.update_name_type(name, T_CONST)

  def opcode(self, name):
    """Add a dictionary entry for a named opcode word to the target rom"""
    if self.vm.DSDeep < 1:
      raise Exception("opcode: stack underflow")
    self.create(name)
    self.append_byte(T_OP)
    opcode = self.vm.T          # take opcode value from top of stack
    self.vm.drop()
    self.append_byte(opcode)
    self.append_byte(RET)
    self.update_name_type(name, T_OP)

  def code(self, name):
    """Start a dictionary entry for named code word to the target rom"""
    link = self.link
    self.create(name)
    self.append_byte(T_OBJ)
    self.append_byte(code)
    self.append_byte(RET)
    self.update_name_type(name, T_OBJ)

  def immediate(self):
    """Modify the last word in the dictionary to be an immediate code word"""
    self.push(self.link + 2)  # address of name field's length
    self.vm.dup()
    self.vm.load_byte()           # load the name's length
    self.vm.add()                 # add length to address of length field
    self.vm.increment()           # last byte of name -> type code
    self.vm.dup()
    self.vm.load_byte()           # verify old type code is T_OBJ
    if self.vm.T != T_OBJ:
      raise Exception("immediate expects T_OBJ", self.vm.T)
    self.vm.T = T_IMM             # check passed, so change type to immediate
    self.vm.swap()
    self.store_byte()

  def update_mode(self):
    """Clear compile mode, but only for final ; of definition"""
    if self.nest_if == 0 and self.nest_for == 0:
      self.mode = MODE_INT

  def compile_literal(self, n):
    """Compile an integer literal of type suitable for size of n"""
    if n >= 0 and n <= 0xff:
      self.append_byte(U8)
      self.append_byte(n)
      return
    if n > 0xff and n <= 0xffff:
      self.append_byte(U16)
      self.append_halfword(n)
    else:
      self.append_byte(I32)
      self.append_word(n)

  def compile_word(self, pos, words):
    """Compile a word, returning (pos) of next unconsumed word"""
    interpreting = self.mode == MODE_INT
    w = words[pos]
    if w == 'hex':                      # hex (immediate)
      self.base = 16
      return pos + 1
    if w == 'decimal':                  # decimal (immediate)
      self.base = 10
      return pos + 1
    if w == 'var':                      # var
      self.var(words[pos + 1])          #   intentionally allow OOR exception
      return pos + 2
    if w == 'const':                    # const
      self.const(words[pos + 1])
      return pos + 2
    if w == 'opcode':                   # opcode
      self.opcode(words[pos + 1])
      return pos + 2
    if w == ':':                        # :
      name = words[pos + 1]
      self.create(name)
      self.append_byte(T_OBJ)
      self.update_name_type(name, T_OBJ)
      self.mode = MODE_COM
      return pos + 2
    if w == ';':                        # ;
      maybe_call = self.DP - 3
      if (maybe_call == self.last_call):
        # Do tail call optimization
        self.push(JMP)
        self.push(maybe_call)
        self.store_byte()
      else:
        # Do a normal return
        self.append_byte(RET)
      self.update_mode()
      return pos + 1
    if w == 'immediate':                # immediate
      self.immediate()
      name = self.link_set[self.link]
      self.update_name_type(name, T_IMM)
      return pos + 1
    if w == 'if{':                      # if{
      self.append_byte(BZ)
      self.push(self.DP)
      self.append_halfword(0)
      self.nest_if += 1
      return pos + 1
    if w == '}if':                      # }if
      self.push(self.DP)
      self.vm.swap()
      self.store_halfword()
      self.nest_if -= 1
      if self.nest_if < 0:
        raise Exception("`}if` without matching `if{`")
      self.last_call = 0                #   prevent glitch with: ` ... }if ;`
      return pos + 1
    if w == 'for{':                     # for{
      self.append_byte(MTR)
      self.push(self.DP)
      self.nest_for += 1
      return pos + 1
    if w == '}for':                     # }for
      self.append_byte(BFOR)
      addr = self.vm.T
      self.vm.drop()
      self.append_halfword(addr)
      self.nest_for -= 1
      if self.nest_for < 0:
        raise Exception("`}for` without matching `for{`")
      self.last_call = 0                #   prevent glitch with: ` ... }for ;`
      return pos + 1
    if w in self.name_set:              # look for word in target's dictionary
      (type_, link) = self.name_set[w]
      param = link + 2 + 1 + len(w) + 1
      if type_ == T_VAR:                # var -> compile U16 param addr
        self.append_byte(U16)
        self.append_halfword(param)
        return pos + 1
      if type_ == T_CONST:              # const -> compile code to push literal
        self.push(param)                #   load param.value
        self.vm.load_word()
        value = self.vm.T
        self.vm.drop()
        self.compile_literal(value)     #   compile param value as literal
        return pos + 1
      if type_ == T_OP:                 # opcode -> append the opcode byte
        self.push(param)
        self.vm.load_byte()
        opcode = self.vm.T
        self.vm.drop()
        self.append_byte(opcode)
        return pos + 1
      if type_ == T_OBJ:                # code -> compile JAL (call) to param
        self.last_call = self.DP
        self.append_byte(JAL)
        self.append_halfword(param)
        return pos + 1
      raise Exception("compile_word: find", w, type_, link)
    try:                                # attempt to parse word as number
      n = int(w, self.base)
      if self.mode == MODE_COM:
        self.compile_literal(n)
      else:
        self.push(n)
      return pos + 1
    except ValueError as e:
      raise Exception("word not in dictionary and not a number:", w) from None


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

def preprocess_mkb(depth, src):
  """Preprocess Markab source code (comments, loads), return array of words"""
  if depth < 1:
    raise Exception('Preprocessor: too much recursion (maybe a `load"` loop?)')
  pos = 0
  words = []
  count = len(src)
  expecting_name = False
  for i in range(count):
    if pos >= count:
      break
    (word, pos) = next_word(pos, src)
    if word == '':
      continue
    if expecting_name:
      # this allows capturing of `: : ... ;` and `: ( ... ;` without problems
      words.append(word)
      expecting_name = False
      continue
    if word == ':' and not expecting_name:
      expecting_name = True
      words.append(word)
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
        words += preprocess_mkb(depth-1, f.read())
    else:
      words.append(word)
  return words

def compile_words(compiler, words, log):
  linelen = 0                  # loop over words, compiling into dictionary
  pos = 0
  for i in range(len(words)):
    if words[i] == ':':
      if i>1 and words[i-1] == ':':
        pass
      else:
        log[0] += "\n"
        linelen = 0
    if i>1 and words[i-2] in ['var', 'const', 'opcode']:
      if i>2 and words[i-3] == ':':
        pass
      else:
        log[0] += "\n"
        linelen = 0
    w = words[i]
    if linelen + len(w) > 78:
      log[0] += f"\n  {w} "
      linelen = 2 + len(w) + 1
    else:
      log[0] += f"{w} "
      linelen += len(w) + 1
    if pos > i:             # compiling a defining word or string can consume
      pass                  # more than one word, so skip words if needed
    else:
      pos = compiler.compile_word(i, words)  # compile and update position

def compile_mkb(src):
  """Compile Markab source code and return bytearray for a Markab VM rom"""
  depth = 5
  words = preprocess_mkb(depth, src)
  compiler = Compiler()
  log = ['']
  try:
    compile_words(compiler, words, log)
  except Exception as e:
    # only print the source code trace in case of an error
    print(log[0])
    raise e
  compiler.patch_context_current_dp()
  with open(SYM_OUT, 'w') as sym:
    # save debug symbols for disassembler
    syms = sorted(compiler.link_set.items())
    syms = [f"{addr} {name}" for (addr, name) in syms]
    sym.write("\n".join(syms)+"\n")
  return compiler.vm.ram[Heap:compiler.DP]

with open(ROM_OUT, 'w') as rom:
  with open(SRC_IN, 'r') as src:
    rom.buffer.write(compile_mkb(src.read()))
