#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab module to simplify creation of IRC bots
#
# To start mkbot, do `python3 -m mkbot` from the parent directory of the one
# which contains this file.
#
import asyncio
import re
import sys
from typing import Callable

import markab_vm as vm
import ircbot


class Irc(ircbot.Irc):
  """Extend ircbot.Irc to add supprt for synchronous terminal IO backend"""

  def __init__(self, nick, name, host, server, port, chan):
    ircbot.Irc.__init__(self, nick, name, host, server, port, chan)
    self.backend_inlet = None
    self.backend_outlet = None
    self.stdout_interrupt = False
    self.stdin_interrupt = False
    self.channick_latch = chan

  def set_backend_inlet(self, backend_inlet: Callable[[str], None]):
    """Register callback for forwarding irc messages to backend inlet"""
    self.backend_inlet = backend_inlet

  def set_backend_outlet(self, backend_outlet: Callable[[], None]):
    """Register calback for reading backend's stdout holding buffer"""
    self.backend_outlet = backend_outlet

  async def send_to_backend(self, message, channick):
    """Override the ircbot.Irc backend to use synchronous terminal style IO"""
    if self.backend_inlet and self.backend_outlet:
      # This is a synchronous call that will block the thread until the backend
      # finishes evaluating the message
      self.backend_inlet(message)
      # At this point, the response should be waiting in a backend buffer. So,
      # read the buffer, format it for irc, and send reply NOTICE to channick
      stdout_buf = vm.drain_stdout().strip()
      for line in stdout_buf.split("\n"):
        if channick == '#mkb':
          # CAUTION! According to the lore, having bots send NOTICE is safer.
          # On main channel, use PRIVMSG because sometimes we will want pdbot
          # to act on the things we send.
          await self.privmsg(channick, line)
        else:
          # In queries, just use the normal channick
          await self.notice(channick, line)
    else:
      self.notice(channick, "[backend inlet/outlet not connected]")


async def irc_main(rom_bytes, max_cycles):
  """Start the VM in irc-bot mode"""
  nick = 'mkbot'
  name = 'mkbot'
  host = 'localhost'         # connecting from localhost
  irc_server = 'localhost'   # ...to ngircd server also on localhost
  irc_port = 6667
  chan = '#mkb'

  # Plumb up interrupt handling and stdin/stdout between VM and irc
  irc = Irc(nick, name, host, irc_server, irc_port, chan)
  irc.set_backend_inlet(vm.irq_rx)
  irc.set_backend_outlet(vm.drain_stdout)

  # Connect to irc
  await irc.connect()
  await irc.join()
  vm._warm_boot(rom_bytes, max_cycles)  # this should return quickly
  await irc.listen()                    # this is the REPL event loop

def main():
  # Load the default rom file
  rom_bytes = b''
  with open(vm.ROM_FILE, 'rb') as f:
    rom_bytes = f.read()

  # Boot the VM configured to use holding buffer for stdout
  vm.reset_state(echo=(not sys.stdin.isatty()), hold_stdout=True)
  asyncio.run(irc_main(rom_bytes, 65535))
