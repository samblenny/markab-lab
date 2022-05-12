# markab-lab

I'm using this repo as a laboratory for experimenting with techniques to build
a new Forth system, which I call Markab Forth. Departures from old Forth
systems include:

1. Markab uses UTF-8 strings. The traditional Forth convention that one
   character is one ASCII-encoded byte is obsolete and not worthy of being
   perpetuated.

2. Stack cells are 32-bits and stack operations assume 32-bit signed integers.
   There are no words for double-cell operations (as used in 16-bit Forths).


## Goals

My general goal is to get a kernel working well with a command line shell, then
add framebuffer and audio support. Eventually, I would like to have a native
linux-amd64 version (with X11 and PulseAudio) and a feature-compatible
cross-platform version in WebAssembly and Javascript.


## Contents

There are a number of amd64 assembly experiments in [asm/amd64/](asm/amd64)

My interpreter shell experiment is in [repl/](repl)
