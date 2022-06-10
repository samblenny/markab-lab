# markab-lab

I'm using this repo as a laboratory for experimenting with techniques to build
a new VM-based, Forth-like, multi-media programming environment.

The point of all this is to have a simple, stable system for working on
long-term small-scale projects in areas like illustration, music, writing, game
dev, and language learning. The primary emphasis is on "simple" and "stable"
because those qualities are important for working on long-term projects without
having to waste a lot of time on fixing things that break and decay.

Similar to how acid-free paper is good for making books that last for a long
time, and climate controlled vaults are good for storing originals of film or
tape recordings, I want a software system for making multi-media projects that,
once finished, can be used, or archived, for many years without decay. This
problem has been solved, mostly, with POSIX and C for text-based programs. But,
making long-term stable programs with graphics and sound is still difficult.
These ideas are not new -- many others are working in similar directions.


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

4. **Programming environment**: A programming environment is an ecosystem of
   tools and documentation for making programs. For text, you need a text
   editor, compiler or interpreter, and a runtime environment (e.g. VM). For
   sound, you also need tools for working with audio samples and maybe MIDI
   notes, effects, filters, or synthesizer parameters. For 2D graphics you might
   need editors for sprites, color themes, font glyphs, vector images, or
   raster images.


## Source Code Contents

1. [repl2/](repl2): Work in progress on a VM emulator in Python with a
   stack-based kernel interpreters, assembler, and compiler running on top
   of the VM. 

2. [repl/](repl): A simple plain-text Forth system with interpreters and
   compiler running on top of a kernel written in amd64 assembly language for
   linux.

3. [asm/amd64/](asm/amd64): Several C and amd64 assembly language experiments
   written to prepare for making the prototype Forth system in [repl/](repl).
