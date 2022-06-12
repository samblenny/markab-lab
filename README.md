# markab-lab

I'm using this repo as a laboratory for experimenting with techniques to build
a new VM-based, Forth-like, multi-media programming environment.

The point of all this is to have a simple, stable system for working on
long-term small-scale projects in areas like illustration, music, writing, game
dev, language learning, and perhaps embedded devices. The primary emphasis is
on "simple" and "stable" because those qualities are important for working on
long-term projects without having to waste a lot of time on fixing things that
break and decay.

Similar to how acid-free paper is good for making books that last for a long
time, and climate controlled vaults are good for storing originals of film or
tape recordings, I want a software system for making multi-media projects that,
once finished, can be used, or archived, for many years without decay. This
problem has been solved, mostly, with POSIX and C for text-based programs. But,
making long-term stable programs with graphics and sound is still difficult.
These ideas are not new -- many others are working in similar directions.


## Markab Is Part of an Art Practice

What I'm doing here is publishing documentation of an art practice as part of a
larger conversation around art and languages in the context of archival
computing, permacomputing, and low-power computing. My intent is that Markab
will inspire people to seek out the experience of working with software tools
that are simple, efficient, and durable.

Markab is not an "Open Source project", and I am not seeking "contributions". I
use an open license because my goals for an archival grade programming
environment require reference implementations that others are free to adapt and
modify.

For now, follows are appreciated (Hi!), and forks are fine. Eventually, I hope
Markab will evolve into more of a community oriented thing, in some form. But,
I need to write a lot of code and documentation before that would be
reasonable.


## Unpacking the Tagline

"VM-based, Forth-like, multi-media programming environment" means:

1. **VM-Based**: The system architecture starts with a portable virtual
   machine (VM) emulator that includes a virtual stack-based CPU and virtual
   peripherals. The point of virtualization is to make it easy to port the
   whole programming environment to new hardware or software platforms by
   implementing a new VM emulator. The point of easy portability is minimizing
   time wasted when computer companies impose incompatible changes to the
   software interfaces required for using their graphics and sound hardware.

2. **Forth-Like**: Forth is hard to define. But, if you go by the measures of
   complying with official Forth standards, or using traditional Forth naming
   schemes, what I'm doing here is not Forth. On the other hand, for the VM,
   I am using a stack CPU with data and return stacks. For the kernel, I am
   using a dictionary linked list, inner and outer interpreters, stack based
   argument passing, and Reverse Polish Notation (RPN) for math operations.
   Those qualities are very much Forth-like.

3. **Multi-Media**: Multi-media refers to programs that can interface with the
   outside world using graphics and sound, in addition to plain text. This also
   includes the idea of working with writing systems that use ideograms or other
   non-latin glyphs. Imagine a language tutor game where you match illustrations
   or sound clips with Kanji ideograms -- that would be a multi-media program.

4. **Programming Environment**: A programming environment is an ecosystem of
   tools and documentation for making programs. For text, you need a text
   editor, compiler or interpreter, and a runtime environment (VM). For sound,
   you also need tools for working with audio samples and maybe MIDI notes,
   effects, filters, or synthesizer parameters. For 2D graphics you might need
   editors for sprites, color themes, font glyphs, vector images, or raster
   images.


## Source Code Contents

1. [repl2/](repl2): Work in progress on a VM emulator in Python with a
   stack-based kernel interpreters, assembler, and compiler running on top
   of the VM.

2. [repl/](repl): A simple plain-text Forth system with interpreters and
   compiler running on top of a kernel written in amd64 assembly language for
   linux.

3. [asm/amd64/](asm/amd64): Several C and amd64 assembly language experiments
   written to prepare for making the prototype Forth system in [repl/](repl).


## Conduct

The conduct policy for Markab is simple: be nice or get banned.

This is an art project, and I'm busy making stuff. I don't have the time or
inclination to engage in internet drama with randos.

On the other hand, to fans and supporters: you have my gratitude.
