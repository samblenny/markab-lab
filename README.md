# markab-lab

You are looking at a code repository that I use for process documentation as
part of an art practice. The code here is either experimental work in progress
or completed experiments that I keep around for future reference.

Markab is the working name for a suite of software tools for exploring ideas in
illustration, music, and games. The point of building my own tools is to work
on long-term projects with less distraction due to buggy, bloated, rapidly
evolving software.

I am particularly interested in tools and techniques that are suitable for
creating archivable digital works using power-efficient computing equipment
running software designed for offline-first usage. If you don't see why those
goals might be appealing, I invite you to forget you ever saw this and go on
with your life.


## Markab Is Part of an Art Practice

What I'm doing here is documenting one aspect of an art practice.

Markab is not an "Open Source project". I am not seeking "contributions". I use
an open license on this code because it is a good fit for my goals around
process documentation and communicating with other artists. What I'm aiming for
here, among other things, is to use tools and techniques that will stand up
well against the passage of time. I want this to be archival grade software.

Modern software development tools tend to embrace VC-funded startup culture.
The startup ecosystem is optimized to seek profit in an economy of planned
obsolescence, rapid hardware upgrades, and paid subscription services. Those
things, by design, lead to fragile systems that suffer from rapid bit rot.

For an individual working on small projects, keeping up with the churn in
modern software tools becomes prohibitively distracting and expensive.

Markab is part of a practice that seeks to bypass chaotic, unsustainable
methods and instead arrange a more reasonable and efficient workflow. The idea
is to have reliable tools, which are easy to maintain, so I can focus on using
the tools to make stuff.


## Source Code Contents

1. [engine/](engine): Work in progress on a game engine with Markab scripting.

2. [repl2/](repl2):  **[inactive]** Markab VM emulator, Markab kernel rom,
   Markab compilers, and Markab language specification. The tools in this
   directory are mostly written in Python, with a little C.

3. [repl/](repl):  **[inactive]** Experimental Forth system with interpreters
   and compiler running on top of a kernel written in amd64 assembly language
   for linux.

4. [asm/amd64/](asm/amd64): **[inactive]** Several C and amd64 assembly language
   experiments written to prepare for making the prototype Forth system in
   [repl/](repl).


## Conduct

The conduct policy for Markab is simple: Be nice or get banned.

Malicious behavior is not welcome here. Examples of such behavior include, but
are not limited to: hate speech, bigotry, trolling, spamming, and promoting
Ponzi schemes.

On the other hand, to friends, fans, and supporters: Welcome! Let's try to get
along and have a good time.
