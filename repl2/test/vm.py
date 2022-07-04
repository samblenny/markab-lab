#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#

from markab_vm import VM
from mkb_autogen import (
  NOP, ADD, SUB, INC, DEC, MUL, DIV, MOD, AND, INV, OR, XOR, SLL, SRL, SRA,
  EQ, GT, LT, NE, ZE, TRUE, FALSE, JMP, JAL, CALL, RET,
  BZ, BFOR, MTR, RDROP, R, PC, ERR, DROP, DUP, OVER, SWAP,
  U8, U16, I32, LB, SB, LH, SH, LW, SW, RESET, CLERR,
  IOD, IODH, IORH, IOKEY, IOEMIT, IODOT, IODUMP, TRON, TROFF,
  MTA, LBA, LBAI,       AINC, ADEC, A,
  MTB, LBB, LBBI, SBBI, BINC, BDEC, B,
)

def p(s):
  print(s, end='')

def test_push_pop():
  v = VM()
  print("=== test.vm.test_push_pop() ===")
  p(".s")
  v._log_ds()
  v._ok_or_err()
  print("( Stack capacity is 18. Trying to push 19th item will clear stack)")
  for i in range(19):
    p(f"{i} ")
    v._push(i)
    p(".s")
    v._log_ds()
    v._ok_or_err()
  print("( This time, the stack won't clear because only 18 items are pushed)")
  for i in range(18):
    p(f"{i} ")
    v._push(i)
    p(".s")
    v._log_ds()
    v._ok_or_err()
  for i in range(19):
    p("drop .s")
    v.drop()
    v._log_ds()
    v._ok_or_err()
  print()

def test_add_subtract():
  v = VM()
  print("=== test.vm.test_add_subtract() ===")
  p("-5 1 2 .s      (  -6 1 2  OK)")
  v._push(-6)
  v._push(1)
  v._push(2)
  v._log_ds()
  v._ok_or_err()
  p("+ .s             (  -6 3  OK)")
  v.add()
  v._log_ds()
  v._ok_or_err()
  p("+ .s               (  -3  OK)")
  v.add()
  v._log_ds()
  v._ok_or_err()
  p("1+ .s              (  -2  OK)")
  v.increment()
  v._log_ds()
  v._ok_or_err()
  p("1- .s              (  -3  OK)")
  v.decrement()
  v._log_ds()
  v._ok_or_err()
  p("drop 7 9 .s       (  7 9  OK)")
  v.drop()
  v._push(7)
  v._push(9)
  v._log_ds()
  v._ok_or_err()
  p("- .s               (  -2  OK)")
  v.subtract()
  v._log_ds()
  v._ok_or_err()
  print()

def test_multiply():
  v = VM()
  print("=== test.vm.test_multiply() ===")
  p("3 -1 .s                         (  3 -1  OK)")
  v._push(3)
  v._push(-1)
  v._log_ds()
  v._ok_or_err()
  p("* .s drop                         (  -3  OK)")
  v.multiply()
  v._log_ds()
  v.drop()
  v._ok_or_err()
  p("-2 hex .s decimal            ( fffffffe  OK)")
  v._push(-2)
  v._log_ds(base=16)
  v._ok_or_err()
  p("dup * .s drop                      (  4  OK)")
  v.dup()
  v.multiply()
  v._log_ds()
  v.drop()
  v._ok_or_err()
  p("hex 7fffffff decimal .s   (  2147483647  OK)")
  v._push(0x7fffffff)
  v._log_ds()
  v._ok_or_err()
  p("dup * .s                           (  1  OK)")
  v.dup()
  v.multiply()
  v._log_ds()
  v._ok_or_err()
  print()

def test_shift():
  v = VM()
  print("=== test.vm.test_shift() ===")
  p("1 31 .s                   (  1 31  OK)")
  v._push(1)
  v._push(31)
  v._log_ds()
  v._ok_or_err()
  p("<< .s              (  -2147483648  OK)")
  v.shift_left_logical()
  v._log_ds()
  v._ok_or_err()
  p("hex .s decimal        (  80000000  OK)")
  v._log_ds(base=16)
  v._ok_or_err()
  p("31 >>> decimal .s           (  -1  OK)")
  v._push(31)
  v.shift_right_arithmetic()
  v._log_ds()
  v._ok_or_err()
  p("hex .s                (  ffffffff  OK)")
  v._log_ds(base=16)
  v._ok_or_err()
  p("1 >> .s decimal       (  7fffffff  OK)")
  v._push(1)
  v.shift_right_logical()
  v._log_ds(base=16)
  v._ok_or_err()
  p("decimal .s          (  2147483647  OK)")
  v._log_ds()
  v._ok_or_err()
  p("30 >> .s                      ( 1  OK)")
  v._push(30)
  v.shift_right_logical()
  v._log_ds()
  v._ok_or_err()
  p("4 << .s                     (  16  OK)")
  v._push(4)
  v.shift_left_logical()
  v._log_ds()
  v._ok_or_err()
  p("2 >> .s                      (  4  OK)")
  v._push(2)
  v.shift_right_logical()
  v._log_ds()
  v._ok_or_err()
  print()

def test_bitwise():
  v = VM()
  print("=== test.vm.test_bitwise() ===")
  print("hex")
  p("ffffffff 3333 and .s drop     (  3333  OK)")
  v._push(0xffffffff)
  v._push(0x3333)
  v.and_()
  v._log_ds(base=16)
  v.drop()
  v._ok_or_err()
  p("55555555 aaaa or .s drop  (  5555ffff  OK)")
  v._push(0x55555555)
  v._push(0xaaaa)
  v.or_()
  v._log_ds(base=16)
  v.drop()
  v._ok_or_err()
  p("55555555 ffff xor .s drop (  5555aaaa  OK)")
  v._push(0x55555555)
  v._push(0xffff)
  v.xor()
  v._log_ds(base=16)
  v.drop()
  v._ok_or_err()
  p("55555555 inv .s drop      (  aaaaaaaa  OK)")
  v._push(0x55555555)
  v.invert()
  v._log_ds(base=16)
  v.drop()
  v._ok_or_err()
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
  v._ok_or_err()
  p("256 65535 .s drop drop  (  256 65536  OK)")
  code = bytearray()
  code.extend([U16, 0x00, 0x01])  # 256
  code.extend([U16, 0xff, 0xff])  # 65535
  code.extend([RET])
  v._warm_boot(code, max_cycles=len(code))
  v._log_ds()
  v.drop()
  v.drop()
  v._ok_or_err()
  p("65536 -1 .s drop drop    (  65536 -1  OK)")
  code = bytearray()
  code.extend([I32, 0x00, 0x00, 0x01, 0x00])  # 65536
  code.extend([I32, 0xff, 0xff, 0xff, 0xff])  # -1
  code.extend([RET])
  v._warm_boot(code, max_cycles=len(code))
  v._log_ds()
  v.drop()
  v.drop()
  v._ok_or_err()
  p("true false .s reset          (  -1 0  OK)")
  v.true_()
  v.false_()
  v._log_ds()
  v.reset()
  v._ok_or_err()
  print()

def test_load_store():
  v = VM()
  print("=== test.vm.test_load_store() ===")
  # =====================================================
  print("( fetch words take 1 argument)")
  p("w@ .s        (  Stack is empty  ERR2)")
  v.load_word()
  v._log_ds()
  v._ok_or_err()
  p("h@ .s        (  Stack is empty  ERR2)")
  v.load_halfword()
  v._log_ds()
  v._ok_or_err()
  p("@ .s         (  Stack is empty  ERR2)")
  v.load_byte()
  v._log_ds()
  v._ok_or_err()
  print("( store words take 2 arguments)")
  p("9000 w! .s             (  9000  ERR2)")
  v._push(9000)
  v.store_word()
  v._log_ds()
  v._ok_or_err()
  p("h! .s                  (  9000  ERR2)")
  v.store_halfword()
  v._log_ds()
  v._ok_or_err()
  p("! .s                   (  9000  ERR2)")
  v.store_byte()
  v._log_ds()
  v._ok_or_err()
  print("drop  OK")
  v.drop()
  # =====================================================
  print("( w! and w@ cover full signed int32 range )")
  print("( note: w@ does sign extension)")
  p("-1 9000 w! 9000 w@               (  -1  OK)")
  v._push(-1)
  v._push(9000)
  v.store_word()
  v._push(9000)
  v.load_word()
  v._log_ds()
  v._ok_or_err()
  p("1 >> 9000 w! 9000 w@ .s  (  2147483647  OK)")
  v._push(1)
  v.shift_right_logical()
  v._push(9000)
  v.store_word()
  v._push(9000)
  v.load_word()
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  # =====================================================
  print("( h! and h@ clip to unsigned uint16 range)")
  print("( note: h@ does not do sign extension)")
  p("-1 9000 h! 9000 h@          (  65535  OK)")
  v._push(-1)
  v._push(9000)
  v.store_halfword()
  v._push(9000)
  v.load_halfword()
  v._log_ds()
  v._ok_or_err()
  p("1 >> 9000 h! 9000 h@ .s     (  32767  OK)")
  v._push(1)
  v.shift_right_logical()
  v._push(9000)
  v.store_halfword()
  v._push(9000)
  v.load_halfword()
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  # =====================================================
  print("( ! and @ clip to unsigned uint8 range)")
  print("( note: @ does not do sign extension)")
  p("-1 9000 ! 9000 @              (  255  OK)")
  v._push(-1)
  v._push(9000)
  v.store_byte()
  v._push(9000)
  v.load_byte()
  v._log_ds()
  v._ok_or_err()
  p("1 >> 9000 ! 9000 @ .s         (  127  OK)")
  v._push(1)
  v.shift_right_logical()
  v._push(9000)
  v.store_byte()
  v._push(9000)
  v.load_byte()
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print()

def test_return_stack():
  v = VM()
  print("=== test.vm.test_return_stack() ===")
  p(".s            (  Stack is empty  OK)")
  v._log_ds()
  v._ok_or_err()
  p(".ret        (  R-Stack is empty  OK)")
  v._log_rs()
  v._ok_or_err()
  p(" 1 2 3 .s              (  1 2 3  OK)")
  v._push(1)
  v._push(2)
  v._push(3)
  v._log_ds()
  v._ok_or_err()
  p(">r >r >r .ret          (  3 2 1  OK)")
  v.move_t_to_r()
  v.move_t_to_r()
  v.move_t_to_r()
  v._log_rs()
  v._ok_or_err()
  p(".s            (  Stack is empty  OK)")
  v._log_ds()
  v._ok_or_err()
  p("rdrop rdrop rdrop .s          (  OK)")
  v.r_drop()
  v.r_drop()
  v.r_drop()
  v._log_ds()
  v._ok_or_err()
  p(".ret        (  R-Stack is empty  OK)")
  v._log_rs()
  v._ok_or_err()
  # =====================================================
  print("( attempt to overflow return stack)")
  print("( note: overflow auto-resets both stacks)")
  p("reset 99 .s                (  99 OK)")
  v.reset()
  v._push(99)
  v._log_ds()
  v._ok_or_err()
  p(".ret        (  R-Stack is empty  OK)")
  v._log_rs()
  v._ok_or_err()
  for i in range(18):
    p(f"{i} >r .ret")
    v._push(i)
    v.move_t_to_r()
    v._log_rs()
    v._ok_or_err()
  p(".s             (  Stack is empty OK)")
  v._log_ds()
  v._ok_or_err()
  # =====================================================
  print("( attempt to underflow return stack)")
  p("reset 99 .s               (  99  OK)")
  v.reset()
  v._push(99)
  v._log_ds()
  v._ok_or_err()
  for i in range(17):
    p(f"{i} >r .ret")
    v._push(i)
    v.move_t_to_r()
    v._log_rs()
    v._ok_or_err()
  for i in range(17):
    p(f"rdrop .ret")
    v.r_drop()
    v._log_rs()
    v._ok_or_err()
  p(".s                           (  99  OK)")
  v._log_ds()
  v._ok_or_err()
  p("rdrop .ret   (  R-Stack is empty  ERR7)")
  v.r_drop()
  v._log_rs()
  v._ok_or_err()
  p(".s               (  Stack is empty  OK)")
  v._log_ds()
  v._ok_or_err()
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
  v._ok_or_err()
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
  v._ok_or_err()
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
  v._ok_or_err()
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
  v._ok_or_err()
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
  v._ok_or_err()
  print()

def test_over_swap():
  v = VM()
  print("=== test.vm.test_over_swap() ===")
  p("1 2 over .s                         (  1 2 1  OK)")
  v._push(1)
  v._push(2)
  v.over()
  v._log_ds()
  v._ok_or_err()
  p("swap .s                             (  1 1 2  OK)")
  v.swap()
  v._log_ds()
  v._ok_or_err()
  print()

def test_instructions_math_logic():
  v = VM()
  print("=== test.vm.test_instructions_math_logic() ===")
  print("( opcode coverage: NOP ADD SUB INC DEC MUL        )")
  print("(    nop    1 dup   + dup    9   - dup   *  .s    )")
  print("ASM{ NOP U8 1 DUP ADD DUP U8 9 SUB DUP MUL IOD }ASM")
  code = bytearray([NOP, U8, 1, DUP, ADD, DUP, U8, 9, SUB, DUP, MUL, IOD, RET])
  p("warmboot .s                                 (  2 49  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: DIV MOD                        )")
  print("(       19    5   /    19    5   % iod            )")
  print("ASM{ U8 19 U8 5 DIV U8 19 U8 5 MOD IOD }ASM")
  code = bytearray([U8, 19, U8, 5, DIV, U8, 19, U8, 5, MOD, IOD, RET])
  p("warmboot .s                                  (  3 4  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: INC DEC    )")
  print("(       3  1+    9  1-  .s    )")
  print("ASM{ U8 3 INC U8 9 DEC IOD }ASM")
  code = bytearray([U8, 3, INC, U8, 9, DEC, IOD, RET])
  p("warmboot .s                                  (  4 8  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: AND INV OR XOR                      )")
  print("( equivalent to: hex 55 inv 7f and dup 80 or 0f xor    )")
  print("ASM{ U8 85 INV U8 127 AND DUP U8 128 OR U8 15 XOR RET }ASM")
  code = bytearray([U8, 85, INV, U8, 127, AND, DUP, U8, 128, OR])
  code.extend([U8, 15, XOR, RET])
  v._warm_boot(code, max_cycles=99)
  print("warmboot  OK")
  p("hex .s decimal                             (  2a a5  OK)")
  v._log_ds(base=16)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: SLL SRL SRA                         )")
  print("( equivalent to: 1 31 << dup 15 >>> dup 16 >>          )")
  print("ASM{  RET }ASM")
  code = bytearray([U8, 1, U8, 31, SLL, DUP, U8, 15, SRA, DUP])
  code.extend([U8, 16, SRL, RET])
  v._warm_boot(code, max_cycles=99)
  print("warmboot  OK")
  p("hex .s decimal             (  8000000 ffff0000 ffff  OK)")
  v._log_ds(base=16)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: EQ                                  )")
  print("( equivalent to: 1 2 =     2 2 =   2 1 =               )")
  print("ASM{ U8 1 U8 2 EQ  U8 2 U8 2 EQ  U8 2 U8 1 EQ  RET }ASM")
  code = bytearray([U8,1,U8,2,EQ, U8,2,U8,2,EQ, U8,2,U8,1,EQ, RET])
  p("warmboot .s                               (  0 -1 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: GT                                  )")
  print("( equivalent to: 1 2 >     2 2 >   2 1 >               )")
  print("ASM{ U8 1 U8 2 GT  U8 2 U8 2 GT  U8 2 U8 1 GT  RET }ASM")
  code = bytearray([U8,1,U8,2,GT, U8,2,U8,2,GT, U8,2,U8,1,GT, RET])
  p("warmboot .s                               (  0 0 -1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: LT                                  )")
  print("( equivalent to: 1 2 <     2 2 <   2 1 <               )")
  print("ASM{ U8 1 U8 2 LT  U8 2 U8 2 LT  U8 2 U8 1 LT  RET }ASM")
  code = bytearray([U8,1,U8,2,LT, U8,2,U8,2,LT, U8,2,U8,1,LT, RET])
  p("warmboot .s                               (  -1 0 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: NE                                  )")
  print("( equivalent to: 1 2 !=    2 2 !=  2 1 !=              )")
  print("ASM{ U8 1 U8 2 NE  U8 2 U8 2 NE  U8 2 U8 1 NE  RET }ASM")
  code = bytearray([U8,1,U8,2,NE, U8,2,U8,2,NE, U8,2,U8,1,NE, RET])
  p("warmboot .s                              (  -1 0 -1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: TRUE FALSE ZE                       )")
  print("(    true dup 0=  false dup 0=     1 DUP 0=            )")
  print("ASM{ TRUE DUP ZE  FALSE DUP ZE  U8 1 DUP ZE RET }ASM")
  code = bytearray([TRUE, DUP, ZE, FALSE, DUP, ZE, U8, 1, DUP, ZE, RET])
  p("warmboot .s                        (  -1 0 0 -1 1 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print()

def test_instructions_jump():
  v = VM()
  print("=== test.vm.test_instructions_jump() ===")
  print("( opcode coverage: JMP                            )")
  print("( assemble tokens to memory starting at address 0 )")
  print("( 0: push 5                                       )")
  print("( 2: jump to 8                                    )")
  print("( 5: push 6, return                               )")
  print("( 8: push 7, jump to 5                            )")
  print("( addr: 0 1   2 3 4  5 6   7  8 9  10 11 12       )")
  print("(               3------------>8-3=5               )")
  print("(                    5-11=-6<---------11          )")
  print("ASM{   U8 5 JMP 5 0 U8 6 RET U8 7 JMP 250 255 }ASM")
  code = bytearray()
  code.extend([U8, 5, JMP, 5, 0, U8, 6, RET, U8, 7, JMP, 250, 255])
  v._warm_boot(code, max_cycles=99)
  print("warmboot  OK")
  p(".s                                         (  5 7 6  OK)")
  v._log_ds()
  v._ok_or_err()
  print()

def test_instructions_jal_call_return():
  v = VM()
  print("=== test.vm.test_instructions_jal_return() ===")
  print("( opcode coverage: JAL RET                        )")
  print("( assemble tokens to memory starting at address 0 )")
  print("(  0: JAL  7=0x0007, or [ 7, 0] little endian     )")
  print("(  3: JAL 10=0x000A, or [10, 0] little endian     )")
  print("(  6: Return                                      )")
  print("(  7: Subroutine: push 9 to data stack, return    )")
  print("( 10: Subroutine: push 5 to data stack, return    )")
  print("( addr: 0 1 2   3 4 5   6  7 8   9 10 11  12      )")
  print("(         1--------------->7-1=6                  )")
  print("(                 4--------------->10-4=6         )")
  print("ASM{  JAL 6 0 JAL 6 0 RET U8 9 RET U8  5 RET }ASM")
  code = bytearray()
  code.extend([JAL, 6, 0, JAL, 6, 0, RET, U8, 9, RET, U8, 5, RET])
  v._warm_boot(code, max_cycles=99)
  print("warmboot  OK")
  p(".s                                           (  9 5  OK)")
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( opcode coverage: CALL                           )")
  print("( addr: 0 1    2  3 4   5   6  *7*  8   9         )")
  print("ASM{   U8 7 CALL U8 9 IOD RET   U8 25 RET }ASM")
  code = bytearray()
  code.extend([U8, 7, CALL, U8, 9, IOD, RET, U8, 25, RET])
  p("warmboot                                    (  25 9  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( test JAL pushes return address properly         )")
  print("( addr: 0 1   2   3 4 5     6   7  *8*            )")
  print("(                   4------------->8-4=4          )")
  print("ASM{   U8 3 MTR JAL 4 0 RDROP RET IORH RET }ASM")
  code = bytearray()
  code.extend([U8, 3, MTR, JAL, 4, 0, RDROP, RET, IORH, RET])
  p("warmboot                                     (  3 6  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("( test CALL pushes return address properly        )")
  print("( addr: 0 1   2  3 4    5     6   7  *8*          )")
  print("ASM{   U8 3 MTR U8 8 CALL RDROP RET IORH RET }ASM")
  code = bytearray()
  code.extend([U8, 3, MTR, U8, 8, CALL, RDROP, RET, IORH, RET])
  p("warmboot                                     (  3 6  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print()

def test_instructions_jz():
  v = VM()
  print("=== test.vm.test_instructions_jz() ===")
  print("( opcode coverage: BZ                                     )")
  print("( equivalent to: 0 dup if{ 7 }if 0 0= dup if{ 8 }IF       )")
  print("( addr: 0 1   2  3 4  5 6  7 8  9  10 11 12 13 14  15     )")
  print("ASM{   U8 0 DUP BZ 3 U8 7 U8 0 ZE DUP BZ  3 U8  8 RET }ASM")
  print("(         0 dup if{       7 }if                           )")
  print("(                              0 0= dup if{          8 }if)")
  code = bytearray()
  code.extend([U8, 0, DUP, BZ, 3, U8, 7, U8, 0, ZE, DUP, BZ, 3])
  code.extend([U8, 8, RET])
  print("warmboot  OK")
  v._warm_boot(code, max_cycles=99)
  p(".s                                        (  0 -1 8  OK)")
  v._log_ds()
  v._ok_or_err()
  print()

def test_instructions_bfor_mtr_r():
  v = VM()
  print("=== test.vm.test_instructions_bfor_mtr_r() ===")
  print("( opcode coverage: BFOR MTR R      )")
  print("(         3 for{  R  }for          )")
  print("( addr: 0 1    2 *3*    4 5   6    )")
  print("ASM{   U8 3  MTR  R  BFOR 2 RET }ASM")
  print("(                        ^^^ 5-3=2 )")
  code = bytearray([U8, 3, MTR, R, BFOR, 2, RET])
  print("warmboot  OK")
  p(".s                                       (  3 2 1 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("(       3 for{ r    1 and    42 emit }for     )")
  print("(addr:  1   2 *3* 4 5   6  7  8      9   10 11)")
  print("ASM{ U8 3 MTR  R U8 1 AND U8 42 IOEMIT BFOR 8 }ASM")
  print("(                                   11-3=8 ^^^)")
  code = bytearray([U8, 3, MTR])
  code.extend([R, U8, 1, AND, U8, 42, IOEMIT, BFOR, 8, RET])
  p(".s                                   (****  1 0 1 0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print()

def test_instructions_drop_dup_over_swap():
  v = VM()
  print("( opcode coverage: DROP DUP OVER SWAP   )")
  print("( equivalent to: 1 2 over dup drop swap )")
  print("ASM{ u8 1 u8 255 over dup drop swap }ASM")
  code = bytearray([U8, 1, U8, 255, OVER, DUP, DROP, SWAP, RET])
  p("warmboot .s                              (  1 1 255  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print()

def test_instructions_u8_sb_lb():
  v = VM()
  print("== vm.test.test_instructions_u8_sb_lb() ===")
  print("( opcode coverage: U16 SB LB             )")
  print("(     512     1    over b! b@            )")
  print("( IR{ 512     1    OVER SB LB }IR        )")
  print("ASM{  U16 0 2 U8 1 OVER SB LB }ASM")
  code = bytearray([U16, 0, 2, U8, 1, OVER, SB, LB, RET])
  p("warmboot .s                                    (  1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("(     512     65530       over B! b@     )")
  print("( IR{ 512     65530       OVER SB LB }IR )")
  print("ASM{  U16 0 2 U8 255 OVER SB LB }ASM")
  code = bytearray([U16, 0, 2, U8, 255, OVER, SB, LB, RET])
  p("warmboot .s                                  (  255  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print()

def test_instructions_u16_sh_lh():
  v = VM()
  print("== vm.test.test_instructions_u16_sh_lh() ===")
  print("( opcode coverage: U16 SH LH             )")
  print("(     512     1    over h! h@            )")
  print("( IR{ 512     1    OVER SH LH }IR        )")
  print("ASM{  U16 0 2 U8 1 OVER SH LH }ASM")
  code = bytearray([U16, 0, 2, U8, 1, OVER, SH, LH, RET])
  p("warmboot .s                                    (  1  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("(     512     65530       over h! h@     )")
  print("( IR{ 512     65530       OVER SH LH }IR )")
  print("ASM{  U16 0 2 U16 250 255 OVER SH LH }ASM")
  code = bytearray([U16, 0, 2, U16, 250, 255, OVER, SH, LH, RET])
  p("warmboot .s                                (  65530  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print()

def test_instructions_i32_sw_lw():
  v = VM()
  print("== vm.test.test_instructions_i32_sw_lw() ===")
  print("( opcode coverage: I32 SW LW RESET               )")
  print("(     512     -6                  over w! w@     )")
  print("( IR{ 512     -6                  OVER SW LW }IR )")
  print("ASM{  U16 0 2 I32 250 255 255 255 OVER SW LW }ASM")
  code = bytearray([U16, 0, 2, I32, 250, 255, 255, 255, OVER, SW, LW, RET])
  p("warmboot .s                                   (  -6  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print("(     512     2147483647          over w! w@     )")
  print("( IR{ 512     2147483647          OVER SW LW }IR )")
  print("ASM{  U16 0 2 I32 255 255 255 127 OVER SW LW }ASM")
  code = bytearray([U16, 0, 2, I32, 255, 255, 255, 127, OVER, SW, LW, RET])
  p("warmboot .s                           (  2147483647  OK)")
  v._warm_boot(code, max_cycles=99)
  v._log_ds()
  v._ok_or_err()
  print("reset  OK")
  v.reset()
  print()


def test_instructions_reset_io():
  v = VM()
  print("=== test.vm.test_instructions_reset_io() ===")
  print("( opcode coverage: RESET IOD             )")
  print("( ---------------------------------------)")
  print("( IOD -- log data stack decimal          )")
  print("(       33 decimal    .s reset           )")
  print("ASM{ U8 33 IOD RESET }ASM")
  code = bytearray([U8, 33, IOD, RESET, RET])
  p("warmboot                                      (  33  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( IODH -- log data stack hex             )")
  print("(       33      hex    .s reset          )")
  print("ASM{ U8 33 IODH RESET }ASM")
  code = bytearray([U8, 33, IODH, RESET, RET])
  p("warmboot                                      (  21  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( IORH -- log return stack hex           )")
  print("(       33  >r iorh reset                )")
  print("ASM{ U8 33 MTR IORH RESET }ASM")
  code = bytearray([U8, 33, MTR, IORH, RESET, RET])
  p("warmboot                                      (  21  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( IOKEY -- echo a line from STDIN        )")
  print("( do: 99 for{                            )")
  print("(      key                               )")
  print("(      0=   if{ rdrop ret }if dup        )")
  print("(      10 = if{ rdrop ret }if            )")
  print("(      emit                              )")
  print("(     }for                               )")
  print("( addr: 0  1   2 *3*                     )")
  print("ASM{   U8 99 MTR IOKEY")
  print("( addr:  4  5  6     7   8 *9*  10 11    )")
  print("        ZE BZ  4 RDROP RET DUP  U8 10")
  print("( addr: 12 13 14    15  16   *17*        )")
  print("        EQ BZ  4 RDROP RET IOEMIT")
  print("( addr:   18 19                          )")
  print("        BFOR 16                       }ASM")
  print("(            ^^^ 19-3=16                 )")
  code = bytearray()
  code.extend([U8, 99, MTR, IOKEY])
  code.extend([ZE, BZ, 3, RDROP, RET, DUP, U8, 10])
  code.extend([EQ, BZ, 3, RDROP, RET, IOEMIT])
  code.extend([BFOR, 16, RET])
  p("warmboot                                       ( test1  OK)")
  v._warm_boot(code, max_cycles=9999)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( IOEMIT -- write to STDOUT              )")
  print("( Write space, space, star               )")
  print("ASM{ U8 32 IOEMIT U8 32 IOEMIT")
  print("     U8 42 IOEMIT RESET           }ASM")
  code = bytearray([U8, 32, IOEMIT, U8, 32, IOEMIT])
  code.extend([U8, 42, IOEMIT, RESET, RET])
  p("warmboot                                       (  *  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( Write utf-8 bytes for U+2708 U+FE0F    )")
  print("ASM{ U8  32 IOEMIT U8  32 IOEMIT")
  print("     U8 226 IOEMIT U8 156 IOEMIT")
  print("     U8 136 IOEMIT U8 239 IOEMIT")
  print("     U8 184 IOEMIT U8 143 IOEMIT")
  print("     RESET                            }ASM")
  code = bytearray()
  code.extend([U8,  32, IOEMIT, U8,  32, IOEMIT])
  code.extend([U8, 226, IOEMIT, U8, 156, IOEMIT])
  code.extend([U8, 136, IOEMIT, U8, 239, IOEMIT])
  code.extend([U8, 184, IOEMIT, U8, 143, IOEMIT])
  code.extend([RESET, RET])
  p("warmboot                                        ( ✈️  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print()

def test_instructions_r_pc():
  v = VM()
  print("=== test.vm.test_instructions_r_pc() ===")
  print("( opcode coverage: R PC                  )")
  print("( ---------------------------------------)")
  print("( R -- Load R, top of return stack       )")
  print("(         5 >r  r  .s reset              )")
  print("ASM{   U8 5 MTR R IOD RESET }ASM")
  code = bytearray([U8, 5, MTR, R, IOD, RESET, RET])
  p("warmboot                                       (  5  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( PC -- Load Program Counter             )")
  print("(      nop nop pc  .s reset              )")
  print("( addr:  0   1  2   3     4              )")
  print("ASM{   NOP NOP PC IOD RESET }ASM")
  code = bytearray([NOP, NOP, PC, IOD, RESET, RET])
  p("warmboot                                       (  3  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print()

def test_instructions_mta_lba_lbai_ainc_adec_a():
  v = VM()
  print("=== test.vm.test_instructions_mta_lbai_ainc_adec_a() ===")
  print("( ---------------------------------------------)")
  print("( MTA -- Move T to register A                  )")
  print("( LBA -- Load Byte via A                       )")
  print("( LBAI -- Load Byte via A, inc A: @a+          )")
  print("(         14  >a  @a+  1- for{  @a   a+   emit )")
  print("( addr: 0  1   2    3   4    5 *6*    7      8 )")
  print("ASM{   U8 13 MTA LBAI DEC  MTR LBA AINC IOEMIT")
  print("(             }for reset                       )")
  print("( addr:    9 10    11  12                      )")
  print("        BFOR  4 RESET RET")
  print("( addr: *13*                                   )")
  print("          7  32 32 72 101 108 108 111 }ASM")
  code = bytearray([U8, 13, MTA, LBAI, DEC, MTR, LBA, AINC, IOEMIT])
  code.extend([BFOR, 4, RESET, RET])
  code.extend([7, 32, 32, 72, 101, 108, 108, 111])
  p("warmboot                                   (  Hello  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( AINC -- Add 1 to A                     )")
  print("( A -- push a copy of A to data stack    )")
  print("(       4  >a   a+ a  .s reset           )")
  print("ASM{ U8 4 MTA AINC A IOD RESET }ASM")
  code = bytearray([U8, 4, MTA, AINC, A, IOD, RESET, RET])
  p("warmboot                                       (  5  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( ADEC -- Subtract 1 from A              )")
  print("(       5  >a   a- a  .s reset           )")
  print("ASM{ U8 5 MTA ADEC A IOD RESET }ASM")
  code = bytearray([U8, 5, MTA, ADEC, A, IOD, RESET, RET])
  p("warmboot                                       (  4  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print()

def test_instructions_mtb_lbb_lbbi_sbbi_binc_bdec_b():
  v = VM()
  print("=== test.vm.test_instructions_mtb_lbb_lbbi_sbbi_binc_bdec_b() ===")
  print("( ---------------------------------------)")
  print("( MTB -- Move T to register B            )")
  print("( SBBI -- Store Byte via B, inc B: !b+   )")
  print("( B -- push a copy of B to data stack    )")
  print("(         1  >b b  !b+ b  !b+ b  !b+     )")
  print("ASM{   U8 1 MTB B SBBI B SBBI B SBBI")
  print("       U8 1 >a a@+ a@+ a@+ IOD RESET }ASM")
  code = bytearray([U8, 1, MTB, B, SBBI, B, SBBI, B, SBBI])
  code.extend([U8, 1, MTA, LBAI, LBAI, LBAI, IOD, RESET, RET])
  p("warmboot                                   (  1 2 3  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ------------------------------------------)")
  print("( LBB -- Load Byte via B                    )")
  print("( LBBI -- Load Byte via B, inc B: @a+       )")
  print("(         8  >b  @b+  @b  .s reset          )")
  print("(     0 1   2    3   4   5     6   7 *8*    )")
  print("ASM{ U8 8 MTB LBBI LBB IOD RESET RET 98 99 }ASM")
  code = bytearray([U8, 8, MTB, LBBI, LBB, IOD, RESET, RET, 98, 99])
  p("warmboot                                   (  98 99  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( BINC -- Add 1 to B                     )")
  print("(       4  >b   b+ b  .s reset           )")
  print("ASM{ U8 8 MTA BINC B IOD RESET }ASM")
  code = bytearray([U8, 8, MTB, BINC, B, IOD, RESET, RET])
  p("warmboot                                       (  9  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print("( ---------------------------------------)")
  print("( BDEC -- Subtract 1 from B              )")
  print("(       5  >b   b- b  .s reset           )")
  print("ASM{ U8 8 MTB BDEC B IOD RESET }ASM")
  code = bytearray([U8, 8, MTB, BDEC, B, IOD, RESET, RET])
  p("warmboot                                       (  7  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
  print()

def test_instructions_err_clerr():
  v = VM()
  print("=== test.vm.test_instructions_err_clerr() ===")
  print("( opcode coverage: ERR CLERR         )")
  print("ASM{ RESET ERR IOD DROP DROP ERR IOD")
  print("     DROP CLERR ERR IOD           }ASM")
  code = bytearray([RESET, ERR, IOD, DROP, DROP, ERR, IOD])
  code.extend(     [DROP, CLERR, ERR, IOD, RET])
  p("warmboot                                 (  0  2  0  OK)")
  v._warm_boot(code, max_cycles=99)
  v._ok_or_err()
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
test_instructions_math_logic()
test_instructions_jump()
test_instructions_jal_call_return()
test_instructions_jz()
test_instructions_bfor_mtr_r()
test_instructions_drop_dup_over_swap()
test_instructions_u8_sb_lb()
test_instructions_u16_sh_lh()
test_instructions_i32_sw_lw()
test_instructions_reset_io()
test_instructions_r_pc()
test_instructions_mta_lba_lbai_ainc_adec_a()
test_instructions_mtb_lbb_lbbi_sbbi_binc_bdec_b()
test_instructions_err_clerr()
