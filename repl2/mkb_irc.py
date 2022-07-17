#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Markab module to simplify creation of IRC bots
#
import asyncio
import re
from typing import Callable

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
    if self.writer is None or self.writer.is_closing():
      print(">>> irc is disconnected >>>")
    else:
      print(message)
      self.writer.write(f"{message}\r\n".encode('utf8'))
      await self.writer.drain()
