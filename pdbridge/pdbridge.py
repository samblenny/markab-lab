#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Network bridge to connect pd-vanilla netsend/netreceive objects to IRC
#
import asyncio
import re

class Irc():
  def __init__(self, nick, name, host, server, port, chan):
    self.nick = nick
    self.name = name
    self.host = host
    self.server = server
    self.port = port
    self.chan = chan
    # Regular expression to match PRIVMSG or NOTICE messages in {chan}
    pattern = f":([^ ]+)![^ ]+ (PRIVMSG|NOTICE) {chan} :(.*)"
    self.wake = re.compile(pattern)

  def set_pd(self, pd):
    self.pd = pd

  async def connect(self):
    reader, writer = await asyncio.open_connection(self.server, self.port)
    self.reader = reader
    self.writer = writer
    await self.send(f"NICK {self.nick}")
    user = f"USER {self.nick} {self.host} {self.server} :{self.name}"
    await self.send(user)
    await self.writer.drain()

  async def join(self):
    self.writer.write(f"JOIN {self.chan}\r\n".encode('utf8'))
    await self.writer.drain()

  async def listen(self):
    while True:
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
    """Parse input line to see if somebody is talking to the bridge"""
    result = self.wake.match(line)
    if result:
      (sender, method, message) = result.group(1, 2, 3)
      if method == 'PRIVMSG':
        if message.lower().startswith("hi"):
          await self.privmsg(f"Hello, {sender}")
        else:
          await self.pd.send(message)

  async def privmsg(self, message):
    await self.send(f"PRIVMSG {self.chan} :{message}")

  async def notice(self, message):
    await self.send(f"NOTICE {self.chan} :{message}")

  async def send(self, message):
    print(message)
    self.writer.write(f"{message}\r\n".encode('utf8'))
    await self.writer.drain()


class Pd():
  def __init__(self, server, port):
    self.server = server
    self.port = port

  def set_irc(self, irc):
    self.irc = irc

  async def connect(self):
    print(f">>> connecting to pd {self.server}:{self.port} >>>")
    reader, writer = await asyncio.open_connection(self.server, self.port)
    print(">>> pd connected >>>")
    self.reader = reader
    self.writer = writer

  async def listen(self):
    while True:
      line = await self.reader.readline()
      line = line.decode('utf8').strip()
      if line == '':
        await asyncio.sleep(0.001)
      else:
        await self.pd.notice(line)

  async def send(self, message):
    self.writer.write(f"{message}\r\n".encode('utf8'))
    await self.writer.drain()


async def main():
  nick = 'pd'
  name = 'Bridge to Pd'
  host = 'localhost'         # connecting from localhost
  irc_server = 'localhost'   # ...to ngircd server also on localhost
  irc_port = 6667
  chan = '#pd'
  pd_server = 'localhost'
  pd_port = 3000             # Pd netreceive docs use 3000, seems good?

  # Create the connection manager objects and cross-link them
  irc = Irc(nick, name, host, irc_server, irc_port, chan)
  pd = Pd(pd_server, pd_port)
  irc.set_pd(pd)
  pd.set_irc(irc)

  # Start the IRC connection
  await irc.connect()
  await irc.join()
  irc_listen_task = asyncio.create_task(irc.listen())  # background task

  # Start the Pd connection
  await pd.connect()
  await pd.listen()


asyncio.run(main())
