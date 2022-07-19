#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab module to simplify creation of IRC bots
#
import asyncio
import re
import sys
from typing import Callable

from markab_vm import VM, ROM_FILE


class Irc():
  def __init__(self, nick, name, host, server, port, chan):
    self.nick = nick
    self.name = name
    self.host = host
    self.server = server
    self.port = port
    self.chan = chan
    self.reader = None
    self.writer = None
    self.rx_callback = None
    self.rx_irq = None
    # Regular expression to match PRIVMSG or NOTICE messages in {chan}
    pattern = f":([^ ]+)![^ ]+ (PRIVMSG|NOTICE) {chan} :(.*)"
    self.wake = re.compile(pattern)

  def set_rx_callback(self, rx_callback: Callable[[str], None]):
    """Register calback function to handle inbound messages"""
    self.rx_callback = rx_callback

  def set_rx_irq(self, rx_irq: Callable[None, None]):
    """Register calback function to raise receive notification interrupt"""
    self.rx_irq = rx_irq

  async def connect(self):
    print(f">>> connecting to irc {self.server}:{self.port} >>>")
    retry = 5
    notice_enable = True
    while True:
      try:
        # if the server is up and listening, this should work right away
        reader, writer = await asyncio.open_connection(self.server, self.port)
        self.reader = reader
        self.writer = writer
        await self.send(f"NICK {self.nick}")
        user = f"USER {self.nick} {self.host} {self.server} :{self.name}"
        await self.send(user)
        await self.writer.drain()
        break
      except OSError:
        # if something went wrong, auto-retry with delay
        if notice_enable:
          print(">>> irc connect failed: check irc server >>>")
          notice_enable = False
        await asyncio.sleep(retry)

  async def join(self):
    await self.send(f"JOIN {self.chan}")

  async def listen(self):
    while True:
      if self.reader is None or self.reader.at_eof():
        # connection is down, so try to reconnect
        await self.connect()
        await self.join()
      line = await self.reader.readline()
      line = line.decode('utf8').strip()
      if line == '':
        await asyncio.sleep(0.001)
      else:
        print(line)
        if line.startswith("PING "):
          await self.pong(line[5:])
        else:
          await self.parse(line)

  async def pong(self, dest):
    await self.send(f"PONG {dest}")

  async def parse(self, line):
    """Parse input line to see if somebody is trying to talk to us"""
    result = self.wake.match(line)
    if result:
      (sender, method, message) = result.group(1, 2, 3)
      if method == 'PRIVMSG':
        if message.lower().startswith("hi"):
          await self.notice(f"Hello, {sender}")
        elif self.rx_callback:
          self.rx_callback(message)
          if self.rx_irq:
            await self.rx_irq()
        else:
          self.notice("[rx_callback not connected]")

  async def privmsg(self, message):
    await self.send(f"PRIVMSG {self.chan} :{message}")

  async def notice(self, message):
    await self.send(f"NOTICE {self.chan} :{message}")

  async def send(self, message):
    """Send irc message with crude truncation at 480 bytes.
    The RFC limit is 512 bytes for a message, but ngircd will cut messages down
    to about 497 bytes. So, I will stay a bit below the lower limit. This way
    of truncation can misbehave badly for long messages that include non-ASCII
    characters. If that becomes an issue, the truncation routine needs to be
    improved to detect Unicode grapheme cluster boundaries. That's a fairly
    involved thing to do, so for now I'll just igore it and hope for the best.
    """
    message = f"{message}".encode('utf8')
    if len(message) > 480:
      message = message[:480] + "[TRUNCATED]\r\n".encode('utf8')
    else:
      message += "\r\n".encode('utf8')
    if self.writer is None or self.writer.is_closing():
      print(">>> irc is disconnected >>>")
    else:
      print(message.decode('utf8').strip())
      self.writer.write(message)
      await self.writer.drain()


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


"""
Load and boot the ROM file when VM is run as a module rather than imported
"""
if __name__ == '__main__':

  # Start by assuming we'll use the default rom file and terminal IO
  rom = ROM_FILE
  args = sys.argv[1:]

  # Check for a command line argument asking for a different rom.
  # For example: `./markab_vm.py hello.rom`
  if (len(args) > 0) and (args[-1].endswith(".rom")):
      rom = args[-1]

  # Make a VM instance
  v = VM(echo=(not sys.stdin.isatty()))

  # Load the rom file
  rom_bytes = b''
  with open(rom, 'rb') as f:
    rom_bytes = f.read()

  # Boot the VM with irc IO
  asyncio.run(irc_main(v, rom_bytes, 65535))
