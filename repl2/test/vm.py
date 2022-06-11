#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#

from markab_vm import VM
from opcodes import (
  NOP, ADD, SUB, MUL, AND, INV, OR, XOR, SLL, SRL, SRA, EQ, GT, LT, NE, ZE,
  JMP, JAL, RET, BZ, DRBLT, MRT, MTR, DROP, DUP, OVER, SWAP,
  U8, U16, I32, LB, SB, LH, SH, LW, SW, RESET, ECALL,
  E_DS, E_RS, E_DSH, E_RSH, E_PC, E_READ, E_WRITE,
)

def p(s):
  print(s, end='')

def test_push_pop():
  v = VM()
  print("=== test.vm.test_push_pop() ===")
  p(".s")
  v._log_ds()
  print("( Stack capacity is 18. Trying to push 19th item will clear stack)")
  for i in range(19):
    p(f"{i} ")
    v._push(i)
    p(".s")
    v._log_ds()
  print("( This time, the stack won't clear because only 18 items are pushed)")
  for i in range(18):
    p(f"{i} ")
    v._push(i)
    p(".s")
    v._log_ds()
  for i in range(19):
    p("drop .s")
    v.drop()
    v._log_ds()
  print()

def test_add_subtract():
  v = VM()
  print("=== test.vm.test_add_subtract() ===")
  p("-5 1 2 .s      (  -6 1 2  OK)")
  v._push(-6)
  v._push(1)
  v._push(2)
  v._log_ds()
  p("+ .s             (  -6 3  OK)")
  v.add()
  v._log_ds()
  p("+ .s               (  -3  OK)")
  v.add()
  v._log_ds()
  p("drop 7 9 .s       (  7 9  OK)")
  v.drop()
  v._push(7)
  v._push(9)
  v._log_ds()
  p("- .s               (  -2  OK)")
  v.subtract()
  v._log_ds()
  print()

def test_multiply():
  v = VM()
  print("=== test.vm.test_multiply() ===")
  p("3 -1 .s                         (  3 -1  OK)")
  v._push(3)
  v._push(-1)
  v._log_ds()
  p("* .s drop                         (  -3  OK)")
  v.multiply()
  v._log_ds()
  v.drop()
  p("-2 hex .s decimal            ( fffffffe  OK)")
  v._push(-2)
  v._log_ds(base=16)
  p("dup * .s drop                      (  4  OK)")
  v.dup()
  v.multiply()
  v._log_ds()
  v.drop()
  p("hex 7fffffff decimal .s   (  2147483647  OK)")
  v._push(0x7fffffff)
  v._log_ds()
  p("dup * .s                           (  1  OK)")
  v.dup()
  v.multiply()
  v._log_ds()
  print()

def test_shift():
  v = VM()
  print("=== test.vm.test_shift() ===")
  p("1 31 .s                   (  1 31  OK)")
  v._push(1)
  v._push(31)
  v._log_ds()
  p("<< .s              (  -2147483648  OK)")
  v.shift_left_logical()
  v._log_ds()
  p("hex .s decimal        (  80000000  OK)")
  v._log_ds(base=16)
  p("31 >>> decimal .s           (  -1  OK)")
  v._push(31)
  v.shift_right_arithmetic()
  v._log_ds()
  p("hex .s                (  ffffffff  OK)")
  v._log_ds(base=16)
  p("1 >> .s decimal       (  7fffffff  OK)")
  v._push(1)
  v.shift_right_logical()
  v._log_ds(base=16)
  p("decimal .s          (  2147483647  OK)")
  v._log_ds()
  p("30 >> .s                      ( 1  OK)")
  v._push(30)
  v.shift_right_logical()
  v._log_ds()
  p("4 << .s                     (  16  OK)")
  v._push(4)
  v.shift_left_logical()
  v._log_ds()
  p("2 >> .s                      (  4  OK)")
  v._push(2)
  v.shift_right_logical()
  v._log_ds()
  print()

def test_bitwise():
  v = VM()
  print("=== test.vm.test_bitwise() ===")
  print("hex")
  p("ffffffff 3333 & .s drop       (  3333  OK)")
  v._push(0xffffffff)
  v._push(0x3333)
  v.and_()
  v._log_ds(base=16)
  v.drop()
  p("55555555 aaaa | .s drop   (  5555ffff  OK)")
  v._push(0x55555555)
  v._push(0xaaaa)
  v.or_()
  v._log_ds(base=16)
  v.drop()
  p("55555555 ffff ^ .s drop   (  5555aaaa  OK)")
  v._push(0x55555555)
  v._push(0xffff)
  v.xor()
  v._log_ds(base=16)
  v.drop()
  p("55555555 ~ .s drop        (  aaaaaaaa  OK)")
  v._push(0x55555555)
  v.invert()
  v._log_ds(base=16)
  v.drop()
  print("decimal")
  print()

def test_literals():
  v = VM()
  print("=== test.vm.test_literals() ===")
  p("0 255 .s drop drop          (  0 255  OK)")
  code = bytearray()
  code.extend([U8, 0, U8, 255, RET])
  v._warm_boot(code, max_cycles=len(code))
  v._log_ds()
  v.drop()
  v.drop()
  p("256 65535 .s drop drop  (  256 65536  OK)")
  code = bytearray()
  code.extend([U16, 0x00, 0x01])  # 256
  code.extend([U16, 0xff, 0xff])  # 65535
  code.extend([RET])
  v._warm_boot(code, max_cycles=len(code))
  v._log_ds()
  v.drop()
  v.drop()
  p("65536 -1 .s drop drop    (  65536 -1  OK)")
  code = bytearray()
  code.extend([I32, 0x00, 0x00, 0x01, 0x00])  # 65536
  code.extend([I32, 0xff, 0xff, 0xff, 0xff])  # -1
  code.extend([RET])
  v._warm_boot(code, max_cycles=len(code))
  v._log_ds()
  v.drop()
  v.drop()
  print()

def test_load_store():
  v = VM()
  print("=== test.vm.test_load_store() ===")
  # =====================================================
  print("( fetch words take 1 argument)")
  p("@ .s         (  Stack is empty  ERR2)")
  v.load_word()
  v._log_ds()
  p("w@ .s        (  Stack is empty  ERR2)")
  v.load_halfword()
  v._log_ds()
  p("b@ .s        (  Stack is empty  ERR2)")
  v.load_byte()
  v._log_ds()
  print("( store words take 2 arguments)")
  p("9000 ! .s              (  9000  ERR2)")
  v._push(9000)
  v.store_word()
  v._log_ds()
  p("w! .s                  (  9000  ERR2)")
  v.store_halfword()
  v._log_ds()
  p("b! .s                  (  9000  ERR2)")
  v.store_byte()
  v._log_ds()
  print("drop  OK")
  v.drop()
  # =====================================================
  print("( @ and ! addresses must be >= 0)")
  p("-1 @ .s                  (  -1  ERR3)")
  v._push(-1)
  v.load_word()
  v._log_ds()
  p("w@ .s                    (  -1  ERR3)")
  v.load_halfword()
  v._log_ds()
  p("b@ .s                    (  -1  ERR3)")
  v.load_byte()
  v._log_ds()
  p("-1 ! .s               (  -1 -1  ERR3)")
  v._push(-1)
  v.store_word()
  v._log_ds()
  p("w! .s                 (  -1 -1  ERR3)")
  v.store_halfword()
  v._log_ds()
  p("b! .s                 (  -1 -1  ERR3)")
  v.store_byte()
  v._log_ds()
  print("drop drop  OK")
  v.drop()
  v.drop()
  # =====================================================
  print("( ! address must be <= 0xffff - 3)")
  p("4 65536 ! .s        (  4 65536  ERR3)")
  v._push(4)
  v._push(65536)
  v.store_word()
  v._log_ds()
  for i in range(4):
    (pad, code) = ("     ", "ERR3") if i<3 else ("","OK")
    stk = f"4 {65535-i}" if i<3 else "Stack is empty"
    p(f"1 - ! .s       {pad}(  {stk}  {code})")
    v._push(1)
    v.subtract()
    v.store_word()
    v._log_ds()
  # =====================================================
  print("( @ address must be <= 0xffff - 3)")
  p("65536 @ .s            (  65536  ERR3)")
  v._push(65536)
  v.load_word()
  v._log_ds()
  for i in range(4):
    (pad, code) = ("", "ERR3") if i<3 else ("      ","OK")
    _t = 65535-i if i<3 else 4
    p(f"1 - @ .s              {pad}(  {_t}  {code})")
    v._push(1)
    v.subtract()
    v.load_word()
    v._log_ds()
  print("drop  OK")
  v.drop()
  # =====================================================
  print("( w! address must be <= 0xffff - 1)")
  p("2 65536 w! .s       (  2 65536  ERR3)")
  v._push(2)
  v._push(65536)
  v.store_halfword()
  v._log_ds()
  for i in range(2):
    (pad, code) = ("     ", "ERR3") if i<1 else ("","OK")
    stk = f"2 {65535-i}" if i<1 else "Stack is empty"
    p(f"1 - w! .s      {pad}(  {stk}  {code})")
    v._push(1)
    v.subtract()
    v.store_halfword()
    v._log_ds()
  # =====================================================
  print("( w@ address must be <= 0xffff - 1)")
  p("65536 w@ .s           (  65536  ERR3)")
  v._push(65536)
  v.load_halfword()
  v._log_ds()
  for i in range(2):
    (pad, code) = ("", "ERR3") if i<1 else ("      ","OK")
    _t = 65535-i if i<1 else 2
    p(f"1 - w@ .s             {pad}(  {_t}  {code})")
    v._push(1)
    v.subtract()
    v.load_halfword()
    v._log_ds()
  print("drop  OK")
  v.drop()
  # =====================================================
  print("( b! address must be <= 0xffff)")
  p("1 65536 b! .s       (  1 65536  ERR3)")
  v._push(1)
  v._push(65536)
  v.store_byte()
  v._log_ds()
  p("1 - b! .s      (  Stack is empty  OK)")
  v._push(1)
  v.subtract()
  v.store_byte()
  v._log_ds()
  print("( b@ address must be <= 0xffff)")
  p("65536 b@ .s           (  65536  ERR3)")
  v.drop()
  v._push(65536)
  v.load_byte()
  v._log_ds()
  p("1 - b@ .s                   (  1  OK)")
  v._push(1)
  v.subtract()
  v.load_byte()
  v._log_ds()
  print("drop  OK")
  v.drop()
  # =====================================================
  print("( ! and @ cover full signed int32 range )")
  print("( note: @ does sign extension)")
  p("-1 9000 ! 9000 @               (  -1  OK)")
  v._push(-1)
  v._push(9000)
  v.store_word()
  v._push(9000)
  v.load_word()
  v._log_ds()
  p("1 >> 9000 ! 9000 @ .s  (  2147483647  OK)")
  v._push(1)
  v.shift_right_logical()
  v._push(9000)
  v.store_word()
  v._push(9000)
  v.load_word()
  v._log_ds()
  print("drop  OK")
  v.drop()
  # =====================================================
  print("( w! and w@ clip to unsigned uint16 range)")
  print("( note: w@ does not do sign extension)")
  p("-1 9000 w! 9000 w@          (  65535  OK)")
  v._push(-1)
  v._push(9000)
  v.store_halfword()
  v._push(9000)
  v.load_halfword()
  v._log_ds()
  p("1 >> 9000 w! 9000 w@ .s     (  32767  OK)")
  v._push(1)
  v.shift_right_logical()
  v._push(9000)
  v.store_halfword()
  v._push(9000)
  v.load_halfword()
  v._log_ds()
  print("drop  OK")
  v.drop()
  # =====================================================
  print("( b! and b@ clip to unsigned uint8 range)")
  print("( note: b@ does not do sign extension)")
  p("-1 9000 b! 9000 b@            (  255  OK)")
  v._push(-1)
  v._push(9000)
  v.store_byte()
  v._push(9000)
  v.load_byte()
  v._log_ds()
  p("1 >> 9000 b! 9000 b@ .s       (  127  OK)")
  v._push(1)
  v.shift_right_logical()
  v._push(9000)
  v.store_byte()
  v._push(9000)
  v.load_byte()
  v._log_ds()
  print("drop  OK")
  v.drop()
  print()

def test_return_stack():
  v = VM()
  print("=== test.vm.test_return_stack() ===")
  p(".s            (  Stack is empty  OK)")
  v._log_ds()
  p(".ret        (  R-Stack is empty  OK)")
  v._log_rs()
  p(" 1 2 3 .s              (  1 2 3  OK)")
  v._push(1)
  v._push(2)
  v._push(3)
  v._log_ds()
  p(">r >r >r .ret          (  3 2 1  OK)")
  v.move_t_to_r()
  v.move_t_to_r()
  v.move_t_to_r()
  v._log_rs()
  p(".s            (  Stack is empty  OK)")
  v._log_ds()
  p("r> r> r> .s            (  1 2 3  OK)")
  v.move_r_to_t()
  v.move_r_to_t()
  v.move_r_to_t()
  v._log_ds()
  p(".ret        (  R-Stack is empty  OK)")
  v._log_rs()
  # =====================================================
  print("( attempt to overflow return stack)")
  print("( note: overflow auto-resets both stacks)")
  p("reset 99 .s                (  99 OK)")
  v.reset()
  v._push(99)
  v._log_ds()
  p(".ret        (  R-Stack is empty  OK)")
  v._log_rs()
  for i in range(18):
    p(f"{i} >r .ret")
    v._push(i)
    v.move_t_to_r()
    v._log_rs()
  p(".s             (  Stack is empty OK)")
  v._log_ds()
  # =====================================================
  print("( attempt to underflow return stack)")
  p("reset 99 .s               (  99  OK)")
  v.reset()
  v._push(99)
  v._log_ds()
  for i in range(17):
    p(f"{i} >r .ret")
    v._push(i)
    v.move_t_to_r()
    v._log_rs()
  for i in range(17):
    p(f"r> .ret")
    v.move_r_to_t()
    v._log_rs()
  p(".s             (  99 16 ... 1 0  OK)")
  v._log_ds()
  p("r> .ret   (  R-Stack is empty  ERR7)")
  v.move_r_to_t()
  v._log_rs()
  p(".s            (  Stack is empty  OK)")
  v._log_ds()
  print()

def test_comparisons():
  v = VM()
  print("=== test.vm.test_comparisons() ===")
  p("reset 1 2 =   2 2 =   2 1 =   .s   (  0 -1 0  OK)")
  v.reset()
  v._push(1)
  v._push(2)
  v.equal()
  v._push(2)
  v._push(2)
  v.equal()
  v._push(2)
  v._push(1)
  v.equal()
  v._log_ds()
  v.reset()
  # =====================================================
  p("reset 1 2 >   2 2 >   2 1 >   .s   (  0 0 -1  OK)")
  v.reset()
  v._push(1)
  v._push(2)
  v.greater_than()
  v._push(2)
  v._push(2)
  v.greater_than()
  v._push(2)
  v._push(1)
  v.greater_than()
  v._log_ds()
  # =====================================================
  p("reset 1 2 <   2 2 <   2 1 <   .s   (  -1 0 0  OK)")
  v.reset()
  v._push(1)
  v._push(2)
  v.less_than()
  v._push(2)
  v._push(2)
  v.less_than()
  v._push(2)
  v._push(1)
  v.less_than()
  v._log_ds()
  # =====================================================
  p("reset 1 2 !=  2 2 !=  2 1 !=  .s  (  -1 0 -1  OK)")
  v.reset()
  v._push(1)
  v._push(2)
  v.not_equal()
  v._push(2)
  v._push(2)
  v.not_equal()
  v._push(2)
  v._push(1)
  v.not_equal()
  v._log_ds()
  # =====================================================
  p("reset  -1 0=    0 0=    1 0=  .s   (  0 -1 0  OK)")
  v.reset()
  v._push(-1)
  v.zero_equal()
  v._push(0)
  v.zero_equal()
  v._push(1)
  v.zero_equal()
  v._log_ds()
  print()

def test_over_swap():
  v = VM()
  print("=== test.vm.test_over_swap() ===")
  p("1 2 over .s                         (  1 2 1  OK)")
  v._push(1)
  v._push(2)
  v.over()
  v._log_ds()
  p("swap .s                             (  1 1 2  OK)")
  v.swap()
  v._log_ds()
  print()

def test_instruction_decode_math_logic():
  v = VM()
  print("=== test.vm.test_instruction_decode_math_logic() ===")
  print("( opcode coverage: NOP ADD SUB MUL                     )")
  print("( equivalent to: 1 dup + dup 9 - dup *                 )")
  print("ASM{ nop U8 1 dup add dup U8 9 sub dup mul ret }ASM")
  code = bytearray([NOP, U8, 1, DUP, ADD, DUP, U8, 9, SUB, DUP, MUL, RET])
  v._warm_boot(code, max_cycles=9)
  print("warmboot  OK")
  p(".s                                          (  2 49  OK)")
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: AND INV OR XOR                      )")
  print("( equivalent to: hex 55 ~ 7f and dup 80 or 0f xor      )")
  print("ASM{ U8 85 inv U8 127 and dup U8 128 or U8 15 xor ret }ASM")
  code = bytearray([U8, 85, INV, U8, 127, AND, DUP, U8, 128, OR])
  code.extend([U8, 15, XOR, RET])
  v._warm_boot(code, max_cycles=99)
  print("warmboot  OK")
  p("hex .s decimal                             (  2a a5  OK)")
  v._log_ds(base=16)
  print("reset  OK")
  v.reset()
  print("( opcode coverage: SLL SRL SRA                         )")
  print("( equivalent to: 1 31 << dup 15 >>> dup 16 >>          )")
  print("ASM{  ret }ASM")
  code = bytearray([U8, 1, U8, 31, SLL, DUP, U8, 15, SRA, DUP])
  code.extend([U8, 16, SRL, RET])
  v._warm_boot(code, max_cycles=99)
  print("warmboot  OK")
  p("hex .s decimal             (  8000000 ffff0000 ffff  OK)")
  v._log_ds(base=16)
  print("reset  OK")
  v.reset()
  print("( opcode coverage: EQ                                  )")
  print("( equivalent to: 1 2 =     2 2 =   2 1 =               )")
  print("ASM{ u8 1 u8 2 eq  u8 2 u8 2 eq  u8 2 u8 1 eq  ret }ASM")
  code = bytearray([U8,1,U8,2,EQ, U8,2,U8,2,EQ, U8,2,U8,1,EQ, RET])
  p("warmboot .s                               (  0 -1 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: GT                                  )")
  print("( equivalent to: 1 2 >     2 2 >   2 1 >               )")
  print("ASM{ u8 1 u8 2 gt  u8 2 u8 2 gt  u8 2 u8 1 gt  ret }ASM")
  code = bytearray([U8,1,U8,2,GT, U8,2,U8,2,GT, U8,2,U8,1,GT, RET])
  p("warmboot .s                               (  0 0 -1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: LT                                  )")
  print("( equivalent to: 1 2 <     2 2 <   2 1 <               )")
  print("ASM{ u8 1 u8 2 lt  u8 2 u8 2 lt  u8 2 u8 1 lt  ret }ASM")
  code = bytearray([U8,1,U8,2,LT, U8,2,U8,2,LT, U8,2,U8,1,LT, RET])
  p("warmboot .s                               (  -1 0 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: NE                                  )")
  print("( equivalent to: 1 2 !=    2 2 !=  2 1 !=              )")
  print("ASM{ u8 1 u8 2 ne  u8 2 u8 2 ne  u8 2 u8 1 ne  ret }ASM")
  code = bytearray([U8,1,U8,2,NE, U8,2,U8,2,NE, U8,2,U8,1,NE, RET])
  p("warmboot .s                              (  -1 0 -1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: ZE                                  )")
  print("( equivalent to:  -1 0=    0 0=      1 0=              )")
  print("ASM{ i32 255 255 255 255 ze  u8 0 ze  u8 1 ze  ret }ASM")
  code = bytearray([I32,255,255,255,255,ZE, U8,0,ZE, U8,1,ZE, RET])
  p("warmboot .s                               (  0 -1 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print()

def test_instruction_decode_jump():
  v = VM()
  print("=== test.vm.test_instruction_decode_jump() ===")
  print("( opcode coverage: JMP                                 )")
  print("( assemble tokens to memory starting at Boot vector 256)")
  print("( 256+0: push 5                                        )")
  print("( 256+2: jump to 256+8                                 )")
  print("( 256+5: push 6, return                                )")
  print("( 256+8: push 7, jump to 256+5                         )")
  print("(   256+:   0 1   2 3 4    5 6   7    8 9  10 11 12    )")
  print("ASM{ U8 5 jmp 8 1 U8 6 ret U8 7 jmp  5  1 }ASM")
  code = bytearray()
  code.extend([U8, 5, JMP, 8, 1, U8, 6, RET, U8, 7, JMP, 5, 1])
  v._warm_boot(code, max_cycles=6)  # U8 jmp U8 jmp U8 ret
  print("warmboot  OK")
  p(".s                                         (  5 7 6  OK)")
  v._log_ds()
  print()

def test_instruction_decode_jal_return():
  v = VM()
  print("=== test.vm.test_instruction_decode_jal_return() ===")
  print("( opcode coverage: JAL RET                             )")
  print("( assemble tokens to memory starting at Boot vector 256)")
  print("( 256+ 0: JAL 256+7=0x0107, or [7, 1] little endian    )")
  print("( 256+ 3: JAL 256+10=0x010A, or [10, 1] little endian  )")
  print("( 256+ 6: Return                                       )")
  print("( 256+ 7: Subroutine: push 9 to data stack, return     )")
  print("( 256+10: Subroutine: push 5 to data stack, return     )")
  print("(   256+:   0 1 2    3  4 5   6    7 8   9   10 11  12 )")
  print("ASM{ jal 7 1 jal 10 1 ret U8 9 ret U8  5 ret }ASM")
  code = bytearray()
  code.extend([JAL, 7, 1, JAL, 10, 1, RET, U8, 9, RET, U8, 5, RET])
  v._warm_boot(code, max_cycles=7)  # jal U8 ret jal U8 ret ret
  print("warmboot  OK")
  p(".s                                           (  9 5  OK)")
  v._log_ds()
  print()

def test_instruction_decode_jz():
  v = VM()
  print("=== test.vm.test_instruction_decode_jz() ===")
  print("( opcode coverage: BZ                                  )")
  print("( equivalent to 0 IF{ 7 }IF 0 0= IF{ 8 }IF             )")
  print("( 256+:  0 1  2 3 4  5 6  7 8  9 10 11 12 13 14  15    )")
  print("ASM{    U8 0 BZ 7 1 U8 7 U8 0 ZE BZ 15  1 U8  8 RET }ASM")
  print("(          0 if{       7 }if                           )")
  print("(                           0 0= if{          8 }if    )")
  code = bytearray()
  code.extend([U8, 0, BZ, 7, 1, U8, 7, U8, 0, ZE, BZ, 15, 1, U8, 8, RET])
  print("warmboot  OK")
  v._warm_boot(code, max_cycles=99)
  p(".s                                        (  0 -1 8  OK)")
  v._log_ds()
  print()

def test_instruction_decode_drblt_mtr_mrt():
  v = VM()
  print("=== test.vm.test_instruction_decode_drblt_mtr_mrt() ===")
  print("( opcode coverage: DRBLT MTR MRT                       )")
  print("(    equivalent to: 3 for{ i }for                      )")
  print("( which expands to: 3 >r :FOR r> dup >r jrnz :FOR      )")
  print("( and assembled at the Boot vector, 256, looks like... )")
  print("( 256+: 0 1   2      3   4   5     6 7  8   9   10  11 )")
  print("ASM{   U8 3 MTR    MRT DUP MTR DRBLT 3  1 RTT DROP RET }ASM")
  print("(         3 for{             i  }FOR                   )")
  print("(           >r :FOR r> dup  >r drblt :FOR  >r drop     )")
  code = bytearray()
  code.extend([U8, 3, MTR, MRT, DUP, MTR, DRBLT, 3, 1, RET])
  print("warmboot  OK")
  v._warm_boot(code, max_cycles=99)
  p(".s                                       (  3 2 1 0  OK)")
  v._log_ds()
  print()

def test_instruction_decode_drop_dup_over_swap():
  v = VM()
  print("( opcode coverage: DROP DUP OVER SWAP   )")
  print("( equivalent to: 1 2 over dup drop swap )")
  print("ASM{ u8 1 u8 255 over dup drop swap }ASM")
  code = bytearray([U8, 1, U8, 255, OVER, DUP, DROP, SWAP, RET])
  p("warmboot .s                              (  1 1 255  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print()

def test_instruction_decode_u8_sb_lb():
  v = VM()
  print("== vm.test.test_instruction_decode_u8_sb_lb() ===")
  print("( opcode coverage: U16 SB LB             )")
  print("(     512     1    over b! b@            )")
  print("( IR{ 512     1    OVER SB LB }IR        )")
  print("ASM{  U16 0 2 U8 1 OVER SB LB }ASM")
  code = bytearray([U16, 0, 2, U8, 1, OVER, SB, LB, RET])
  p("warmboot .s                                    (  1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("(     512     65530       over B! b@     )")
  print("( IR{ 512     65530       OVER SB LB }IR )")
  print("ASM{  U16 0 2 U8 255 OVER SB LB }ASM")
  code = bytearray([U16, 0, 2, U8, 255, OVER, SB, LB, RET])
  p("warmboot .s                                  (  255  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print()

def test_instruction_decode_u16_sh_lh():
  v = VM()
  print("== vm.test.test_instruction_decode_u16_sh_lh() ===")
  print("( opcode coverage: U16 SH LH             )")
  print("(     512     1    over h! h@            )")
  print("( IR{ 512     1    OVER SH LH }IR        )")
  print("ASM{  U16 0 2 U8 1 OVER SH LH }ASM")
  code = bytearray([U16, 0, 2, U8, 1, OVER, SH, LH, RET])
  p("warmboot .s                                    (  1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("(     512     65530       over h! h@     )")
  print("( IR{ 512     65530       OVER SH LH }IR )")
  print("ASM{  U16 0 2 U16 250 255 OVER SH LH }ASM")
  code = bytearray([U16, 0, 2, U16, 250, 255, OVER, SH, LH, RET])
  p("warmboot .s                                (  65530  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print()

def test_instruction_decode_i32_sw_lw():
  v = VM()
  print("== vm.test.test_instruction_decode_i32_sw_lw() ===")
  print("( opcode coverage: I32 SW LW RESET               )")
  print("(     512     -6                  over w! w@     )")
  print("( IR{ 512     -6                  OVER SW LW }IR )")
  print("ASM{  U16 0 2 I32 250 255 255 255 OVER SW LW }ASM")
  code = bytearray([U16, 0, 2, I32, 250, 255, 255, 255, OVER, SW, LW, RET])
  p("warmboot .s                                   (  -6  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print("(     512     2147483647          over w! w@     )")
  print("( IR{ 512     2147483647          OVER SW LW }IR )")
  print("ASM{  U16 0 2 I32 255 255 255 127 OVER SW LW }ASM")
  code = bytearray([U16, 0, 2, I32, 255, 255, 255, 127, OVER, SW, LW, RET])
  p("warmboot .s                           (  2147483647  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  print("reset  OK")
  v.reset()
  print()


def test_instruction_decode_reset_ecall():
  v = VM()
  print("=== test.vm.test_instruction_decode_ecall() ===")
  print("( opcode coverage: RESET ECALL")
  print("( ---------------------------------------)")
  print("( E_DS ECALL -- log data stack decimal   )")
  print("(       33 decimal    .s reset           )")
  print("ASM{ U8 33 U8 E_DS ECALL RESET }ASM")
  code = bytearray([U8, 33, U8, E_DS, ECALL, RESET, RET])
  p("warmboot                                      (  33  OK)")
  v._warm_boot(code, max_cycles=99)
  print("( ---------------------------------------)")
  print("( E_DSH ECALL -- log data stack hex      )")
  print("(       33      hex    .s reset          )")
  print("ASM{ U8 33 U8 E_DSH ECALL RESET }ASM")
  code = bytearray([U8, 33, U8, E_DSH, ECALL, RESET, RET])
  p("warmboot                                      (  21  OK)")
  v._warm_boot(code, max_cycles=99)
  print("( ---------------------------------------)")
  print("( E_RS ECALL -- log return stack decimal )")
  print("(       33  >r decimal   .ret reset      )")
  print("ASM{ U8 33 MTR U8 E_RS  ECALL RESET }ASM")
  code = bytearray([U8, 33, MTR, U8, E_RS, ECALL, RESET, RET])
  p("warmboot                                      (  33  OK)")
  v._warm_boot(code, max_cycles=99)
  print("( ---------------------------------------)")
  print("( E_RSH ECALL -- log return stack hex    )")
  print("(       33  >r      hex  .ret reset      )")
  print("ASM{ U8 33 MTR U8 E_RSH ECALL RESET }ASM")
  code = bytearray([U8, 33, MTR, U8, E_RSH, ECALL, RESET, RET])
  p("warmboot                                      (  21  OK)")
  v._warm_boot(code, max_cycles=99)
  print("( ---------------------------------------)")
  print("( E_PC ECALL -- push Program Counter     )")
  print("(         E_PC ecall            .s reset )")
  print("( 256+: 0    1     2  3    4     5     6 )")
  print("ASM{   U8 E_PC ECALL U8 E_DS ECALL RESET }ASM")
  code = bytearray([U8, E_PC, ECALL, U8, E_DS, ECALL, RESET, RET])
  p("warmboot                                     (  259  OK)")
  v._warm_boot(code, max_cycles=99)
  print("( ---------------------------------------)")
  print("( E_READ ECALL -- read byte from STDIN   )")
  print("( TODO: write test for E_READ            )")
  print("                                                     ( )")
  print("( ---------------------------------------)")
  print("( E_WRITE ECALL -- write to STDOUT       )")
  print("( TODO: write test for E_WRITE           )")
  print("                                                     ( )")
  print()

  
test_push_pop()
test_add_subtract()
test_multiply()
test_shift()
test_bitwise()
test_literals()
test_load_store()
test_return_stack()
test_comparisons()
test_over_swap()
test_instruction_decode_math_logic()
test_instruction_decode_jump()
test_instruction_decode_jal_return()
test_instruction_decode_jz()
test_instruction_decode_drblt_mtr_mrt()
test_instruction_decode_drop_dup_over_swap()
test_instruction_decode_u8_sb_lb()
test_instruction_decode_u16_sh_lh()
test_instruction_decode_i32_sw_lw()
test_instruction_decode_reset_ecall()
