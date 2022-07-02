# markab-lab

You are looking at a code repository that I use for process documentation as
part of an art practice. The code here is either experimental work in progress
or completed experiments that I keep around for future reference.

Markab is the working name for a suite of simple, focused software tools I'm
building to explore ideas in illustration and music. The point of building my
own tools is to work on long-term projects with less distraction due to buggy,
bloated, rapidly evolving software.

I am particularly interested in tools and techniques that are suitable for
creating archivable digital works using power-efficient computing equipment
running software designed for offline-first usage. If you don't see why those
goals might be appealing, I invite you to forget you ever saw this and go on
with your life. On the other hand, if you're curious, read on.


## Markab Is Part of an Art Practice

What I'm doing here is documenting one aspect of an art practice.

Markab is not an "Open Source project". I am not seeking "contributions". I use
an open license on this code because it is a good fit for my goals around
archivable digital works.

By "archivable", I mean capable of being stored, retrieved, viewed, and
interacted with long into the future with a reasonable level of care and
effort. By "reasonable level", I mean that things are arranged to minimize the
time and expense needed to keep software tools working today as well as they
worked last month, last year, or ten years ago.

Modern software tools tend to embrace the culture and mindset popularized by
VC-funded software startups. In that value system, buggy software with rapid
upgrade cycles and dependencies on chaotic online package management systems is
considered normal. This approach leads to many things rapidly breaking
(bitrot), either due to planned obsolescence or incidental side effects.

With popular modern software development frameworks, code from perhaps as
little as one year ago will commonly not build and run without modifications to
compensate for recent platform changes. In many cases, that process will
require purchase of new hardware in order to run the latest developer tools.
Developers commonly complain about their old projects no longer working.

For an individual working on small projects, keeping up with the churn in
modern software tools becomes prohibitively distracting and expensive. Markab
is about opting out of that chaotic software culture and, instead, using tools
designed for long-term stability and low-intensity resource use. My goal is to
have tools that work reliably so I can focus on using them to make stuff.


## Source Code Contents

1. [repl2/](repl2): Work in progress on Markab VM emulator, Markab kernel rom,
   and Markab compilers.

2. [markab-language/](markab-language): Work in progress on Markab language
   spec and text editor syntax highlighting plugins.

3. [repl/](repl): Experimental Forth system with interpreters and compiler
   running on top of a kernel written in amd64 assembly language for linux.

4. [asm/amd64/](asm/amd64): Several C and amd64 assembly language experiments
   written to prepare for making the prototype Forth system in [repl/](repl).


## Conduct

The conduct policy for Markab is simple: be nice or get banned.

This is an art project, and I'm busy making stuff. I don't have the time or
inclination to engage in internet drama with randos.

On the other hand, to friends, fans, and supporters: you have my gratitude.
