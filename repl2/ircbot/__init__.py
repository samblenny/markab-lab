# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
"""
Irc is a class to handle the irc messaging portion of implementing an irc bot.
"""
import asyncio
import random
import re

class Irc():
  def __init__(self, nick, name, host, server, port, chan):
    """Initialize irc connection details and message filters"""
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
    # Prepare regular expressions for use in routing inbound messages
    re_privmsg = f":([^ ]+)![^ ]+ PRIVMSG ({chan}|{nick}) :(.*)"
    re_greet = f"([Hh](i|ello))[^a-zA-Z_-]*$"
    re_to_me = f"({nick}[,:]) (.*)"
    self.re_privmsg = re.compile(re_privmsg)
    self.re_greet = re.compile(re_greet)
    self.re_to_me = re.compile(re_to_me)

  async def route(self, line):
    """Route incoming messages to a suitable handler"""
    # Route 0: Message is not a PRIVMSG in our channel or query -> Ignore it
    re_privmsg = self.re_privmsg.match(line)
    if not re_privmsg:
      return
    # PRIVMSG routing considers sender, channel, and body contents
    (sender, channick, body) = re_privmsg.group(1, 2, 3)
    # Route 1: General greeting in channel or query -> Answer it
    re_greet = self.re_greet.match(body)
    if re_greet:
      salutation = random.choice(['hi', 'hello', 'Hi', 'Hello'])
      pause = random.normalvariate(0.6, 0.1)
      await asyncio.sleep(pause)
      if channick == self.nick:
        # In a query addressed to me, reply should be addressed to sender
        channick = sender
      await self.notice(channick, f"{salutation}, {sender}")
      return
    # Route 2: Query -> Forward message body to backend
    if channick == self.nick:
      await self.send_to_backend(body, sender)  # query reply goes to sender!
      return
    # Route 3: Addressed in channel -> Forward tail of message body to backend
    re_to_me = self.re_to_me.match(body)
    if (channick == self.chan) and re_to_me:
      tail = re_to_me.group(2)
      await self.send_to_backend(tail, channick)
      return

  async def send_to_backend(self, body, channick):
    """Send message to the bot's backend handler. Subclasses can override this
    method to wait for a reply, do async to sync domain crossing, or whatever.
    """
    await self.backend.send(body, channick)

  def set_backend(self, sender):
    """Register a backend sender object that provides a send(str) method"""
    self.backend = sender

  async def connect(self):
    """Connect to the irc server"""
    print(f">>> connecting to irc {self.server}:{self.port} >>>")
    retry = 5
    notice_enable = True    # <- arm the one-shot error message mechanism
    while True:
      try:
        # When the server is up and listening, this should work right away
        reader, writer = await asyncio.open_connection(self.server, self.port)
        self.reader = reader
        self.writer = writer
        await self.send(f"NICK {self.nick}")
        user = f"USER {self.nick} {self.host} {self.server} :{self.name}"
        await self.send(user)
        await self.writer.drain()
        break
      except OSError:
        # If something went wrong, auto-retry with delay
        if notice_enable:
          print(">>> irc connect failed: check irc server >>>")
          # Disable further retry notifications to avoid log spam
          notice_enable = False
        await asyncio.sleep(retry)

  async def join(self):
    """Join our irc channel"""
    await self.send(f"JOIN {self.chan}")

  async def listen(self):
    """Start the event loop to listen for, and respond to, inbound messages"""
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
        if line.startswith("PING "):
          await self.send(f"PONG {line[5:]}")
        else:
          print(line)
          await self.route(line)

  async def notice(self, chan, message):
    """Send an irc NOTICE (most bot messages should generally use this)"""
    if not chan:
      chan = self.chan
    await self.send(f"NOTICE {chan} :{message}")

  async def privmsg(self, chan, message):
    """Send an irc PRIVMSG (CAUTION! using this can create feedback loops)"""
    if not chan:
      chan = self.chan
    await self.send(f"PRIVMSG {chan} :{message}")

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
