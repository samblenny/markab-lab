<!-- Copyright (c) 2023 Sam Blenny -->
<!-- SPDX-License-Identifier: CC-BY-NC-SA-4.0 -->

# markab-lab

Markab is the working name for a suite of software tools for exploring ideas in
illustration, music, and games. This is an art project.

Markab is not an "Open Source project", and I am not "seeking contributions".
To the extent that I use open licenses on markab code, that is because doing so
is a good fit for my goals around process documentation.

This repository functions as a workspace where I play around with new ideas.
Usually, when I finish with something, I will leave its directory in place and
start a new directory for the next experiment.


## Source Code Contents

1. [engine/](engine): Work in progress on a game engine. This is my second game
   engine experiment, and I think it may veer into more of a 3D WebGL thing.
   Earlier, I'd planned to make a 2D sprite engine, but I'm feeling more into
   the idea of simple 3D now that I've written a couple shaders.

2. [old_engine/](old_engine): **[inactive]** Initial experiment game engine
   with Markab scripting. This is implemented in HTML, Javascript, CSS, and C
   compiled to WebAssembly, with a 2D WebGL GUI. There's a bunch of code for
   walking and running input by gamepad or WASD keyboard equivalents. There's
   also a bunch of code and tests for a Markab Script compiler, but that turned
   out to be a lot less useful than I had imagined. Just writing in ANSI C
   seems to work pretty well.

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

Malicious behavior is not welcome here.


## License

The markab project incudes code and other creative works released under either
the MIT or CC BY-NC-SA licenses, depending on the nature of the work. For
specifics, look at the SPDX license header comments.

This README is licensed under a Creative Commons
Attribution-NonCommercial-ShareAlike 4.0 International License.

See [LICENSE_CCBY-NC-SA](LICENSE_CCBY-NC-SA) in this repo for a plaintext copy
of the license or https://creativecommons.org/licenses/by-nc-sa/4.0/ for the
online html version.

See [LICENSE_MIT](LICENSE_MIT) in this repo for the text of the MIT license.
