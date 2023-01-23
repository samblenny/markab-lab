<!-- Copyright (c) 2022 Sam Blenny -->
<!-- SPDX-License-Identifier: MIT -->

# Repl2

**2023-01-23: This experiment is currently inactive**

This directory has a commandline style stack machine VM, bootstrap compiler,
kernel+compiler, REPL rom image, and lots of tests. The VM and bootstrap
compiler are implemented in Python. There is a language specification with
syntax highlighting plugins for vim and emacs in
[./markab-language](markab-language).

The [./ircbot](ircbot), [./mkbot](mkbot), and [./pdbot](pdbot) directories are
for an experiment with using irc bots to connect a Markab REPL session with Pd
(Pure Data) to interactively create audio patches.

This is like a laboratory. Things you encounter here may change suddenly, or
they may be stable for a long time. It just depends on how the work unfolds.
