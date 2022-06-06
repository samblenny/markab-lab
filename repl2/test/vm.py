#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#

from markab_vm import VMTask


def test_vm_push_pop():
  v = VMTask()
  print(".s", end='')
  v.dotS()
  for i in range(18):
    print(f"{i} ", end='')
    v.push(i)
    print(".s", end='')
    v.dotS()
  for i in range(18):
    print("drop .s", end='')
    v.drop()
    v.dotS()


test_vm_push_pop()
