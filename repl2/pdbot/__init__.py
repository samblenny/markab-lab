# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
"""
Pdbot is an irc bot to connect pd-vanilla netsend/netreceive objects to IRC.

To start pdbot, run `python3 -m pdbot` from a terminal.
"""
import asyncio
import random
import re

from ircbot import Irc


class Pd():
  def __init__(self, server, port, irc_chan):
    self.server = server
    self.port = port
    self.irc_chan = irc_chan
    self.reader = None
    self.writer = None

  def set_irc(self, irc):
    self.irc = irc

  async def connect(self, chan):
    print(f">>> connecting to pd {self.server}:{self.port} >>>")
    retry = 5
    notice_enable = True
    while True:
      try:
        # if the patch is loaded and listening, this should work right away
        reader, writer = await asyncio.open_connection(self.server, self.port)
        self.reader = reader
        self.writer = writer
        await self.irc.notice(chan, "[pd is connected]")
        break
      except OSError:
        # if something went wrong, auto-retry with delay
        if notice_enable:
          await self.irc.notice(chan, "[pd is disconnected: check patch]")
          notice_enable = False
        await asyncio.sleep(retry)

  async def listen(self):
    while True:
      if self.reader is None or self.reader.at_eof():
        # connection to pd is down, so try to connect
        await self.connect(self.irc_chan)
      line = await self.reader.readline()
      line = line.decode('utf8').strip()
      if line == '':
        await asyncio.sleep(0.001)
      else:
        await self.backend.notice(line)

  async def send(self, message, channick):
    if self.writer is None or self.writer.is_closing():
      await self.irc.notice(channick, "[pd is disconnected: check patch]")
    else:
      self.writer.write(f"{message}\r\n".encode('utf8'))
      await self.writer.drain()


async def main():
  nick = 'pdbot'
  name = 'Pd Bridge bot'
  host = 'localhost'         # connecting from localhost
  irc_server = 'localhost'   # ...to ngircd server also on localhost
  irc_port = 6667
  pd_server = 'localhost'
  pd_port = 3000             # Pd netreceive docs use 3000, seems good?
  chan = '#mkb'

  # Create the connection manager objects and cross-link them
  irc = Irc(nick, name, host, irc_server, irc_port, chan)
  pd = Pd(pd_server, pd_port, chan)
  irc.set_backend(pd)
  pd.set_irc(irc)

  # Start the IRC connection
  await irc.connect()
  await irc.join()
  irc_listen_task = asyncio.create_task(irc.listen())  # background task

  # Start the Pd connection with auto-reconnect
  await pd.listen()
