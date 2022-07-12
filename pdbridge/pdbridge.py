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
      print(">>> wake >>>", result.group(1, 2, 3))
      if method == 'PRIVMSG':
        if message.lower().startswith("hi"):
          await self.privmsg(f"Hello, {sender}")

  async def privmsg(self, message):
    await self.send(f"PRIVMSG {self.chan} :{message}")

  async def send(self, message):
    print(message)
    self.writer.write(f"{message}\r\n".encode('utf8'))
    await self.writer.drain()


async def main():
  nick = 'pd'
  name = 'Bridge to Pd'
  host = 'localhost'     # connecting from localhost
  server = 'localhost'   # ...to ngircd server also on localhost
  port = 6667
  chan = '#pd'
  irc = Irc(nick, name, host, server, port, chan)
  await irc.connect()
  await irc.join()
  await irc.listen()

asyncio.run(main())
