#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#

from markab_vm import VMTask

def p(s):
  print(s, end='')

def test_push_pop():
  v = VMTask()
  print("=== test.vm.test_push_pop() ===")
  p(".s")
  v.dotS()
  for i in range(19):
    p(f"{i} ")
    v.push(i)
    p(".s")
    v.dotS()
  for i in range(19):
    p("drop .s")
    v.drop()
    v.dotS()

def test_plus_minus():
  v = VMTask()
  print("=== test.vm.test_plus_minus() ===")
  p("-5 1 2 .s")
  v.push(-5)
  v.push(1)
  v.push(2)
  v.dotS()
  p("+ .s")
  v.plus()
  v.dotS()
  p("+ .s")
  v.plus()
  v.dotS()
  p("drop 7 9 .s")
  v.drop()
  v.push(7)
  v.push(9)
  v.dotS()
  p("- .s")
  v.minus()
  v.dotS()


test_push_pop()
test_plus_minus()
