# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
"""
Pdbot is an irc bot to connect pd-vanilla netsend/netreceive objects to IRC.

To start pdbot, run `python3 -m pdbot` from a terminal.
"""

if __name__ == '__main__':
  import asyncio
  from pdbot import main
  asyncio.run(main())
