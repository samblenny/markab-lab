#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# MarkabForth bootstrap compiler
#
import tokens   # get_token(opcode), get_opcode(token)
import mem_map  # get_addr(name)


ROM_FILE = 'kernel.bin'


with open(ROM_FILE, 'w') as f:
  f.write("")

print("TODO: write a bootstrap compiler")

