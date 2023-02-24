# markab-lab

Markab is the working name for a suite of software tools for exploring ideas in
illustration, music, and games as part of my art practice.

This code repository, as a part of the markab project, is specifically for
process documentation of day-to-day work. Code here is either experimental work
in progress or completed experiments that I keep around for future reference.

A lot of what I do involves relatively low-level DIY tool-building on top of
APIs, compilers, and tools maintained by organizations that have a track record
of prioritizing long term stability. I like Debian. I like ANSI C. I like make.
I like standardized web APIs with good cross-platform browser support.

In terms of politics and aesthetics, I prefer to approach computing in ways
that minimize environmental damage, minimize waste, and, generally, minimize
harm to society. I'm interested in making good use of surplus computers that
are still capable of running efficiently written software. I'm interested in
software designed to be usable offline and on a small power budget. Generally,
I'm in favor of wasting fewer CPU cycles and in favor of spending the cycles we
do use on things that are worthwhile and beneficial. I hope my work inspires
people to be less wasteful.


## Markab Is an Art Project

What I'm doing here is mainly about documenting an art practice.

Markab is not an "Open Source project", and I am not "seeking contributions".
To the extent that I use open licenses on markab code, that is because doing so
is a good fit for my goals around process documentation and communicating with
other artists. What I'm aiming for here, among other things, is to use tools
and techniques that will stand up well against the passage of time.

For an individual working on small projects, keeping up with the churn in
modern software development tools and practices becomes prohibitively
distracting and expensive. Building my own tools allows me to maintain a
workflow that is more pleasant and manageable. The idea is to arrange for
reliable tools so I can focus on using those tools to make stuff.


## Source Code Contents

1. [engine/](engine): Work in progress on a game engine. This is my second game
   engine experiment, and I think it may veer into more of a 3D WebGL thing.
   Earlier, I'd planned to make a 2D sprite engine, but I'm feeling more into
   the idea of simple 3D now that I've written a couple shaders.

2. [old_engine/](old_engine): **[inactive]** Initial experiment game engine
   with Markab scripting. This has a Wasm + 2D WebGL front-end GUI with a back
   end Wasm module written in C. There's also a bunch of code and tests for a
   Markab Script compiler, but that turned out to be a lot less useful than I
   had imagined. Just writing in ANSI C seems to work pretty well.

3. [repl2/](repl2):  **[inactive]** Markab VM emulator, Markab kernel rom,
   Markab compilers, and Markab language specification. The tools in this
   directory are mostly written in Python, with a little C.

4. [repl/](repl):  **[inactive]** Experimental Forth system with interpreters
   and compiler running on top of a kernel written in amd64 assembly language
   for linux.

5. [asm/amd64/](asm/amd64): **[inactive]** Several C and amd64 assembly language
   experiments written to prepare for making the prototype Forth system in
   [repl/](repl).


## Conduct

The conduct policy for Markab is simple: Be nice or get banned.

Malicious behavior is not welcome here. Examples of such behavior include, but
are not limited to: hate speech, bigotry, trolling, spamming, and promoting
Ponzi schemes.

On the other hand, to friends, fans, and supporters: Welcome! Let's try to get
along and have a good time.
