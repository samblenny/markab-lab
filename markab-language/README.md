<!-- Copyright (c) 2022 Sam Blenny -->
<!-- SPDX-License-Identifier: MIT -->

# Markab Language

This page documents the Markab programming language. Or, more accurately,
that's the plan, which is still in the early stages of implementation.


## CPU opcodes, ECALL constants, and core words

These are instruction opcode mnemonics used by the Markab assembler to generate
bytecode for the Markab virtual machine:

```
NOP ADD SUB INC DEC MUL AND INV OR XOR
SLL SRL SRA
EQ GT LT NE ZE TRUE FALSE JMP JAL RET
BZ DRBLT MRT MTR RDROP R PC DROP DUP OVER SWAP
U8 U16 I32 LB SB LH SH LW SW RESET
IOD IOR IODH IORH IOKEY IOEMIT
MTA LBA LBAI      AINC ADEC A
MTB LBB LBBI SBBI BINC BDEC B MTX X MTY Y
```

The `U8`, `LB`, `SH`, ... opcodes are for moving different widths of data
between registers and memory. The numbers 8, 16, and 32 refer to 8 bits (1
byte), 16 bits (2 bytes), or 32 bits (4 bytes). `L` stands for load, and `S`
stands for store. `B` stands for byte (8 bits), `H` stands for halfword (16
bits), and `W` stands for word (32-bits). Byte order is little-endian.

The `IO..` opcodes do terminal input/output (IO) operations like debug printing
stack contents to the terminal, getting a byte of keyboard input, or emitting a
byte of output.

The `MTA` (Move T to A) and `LBAI` (Load Byte from A with Increment) opcodes
are for loading a byte from an address stored in the VM's `A` register with an
automatically incrementing address. `MTB` and `SBBI` are similar for storing a
byte to the address in `B`. These are intended for loops that copy, move, or
process strings of bytes. `A` and `B` can also be used as local variables,
along with `X` and `Y`.

Markab is a Forth-like stack language. The usage of "words", "vocabulary",
"dictionary", and "immediate words" here borrow the meanings of those terms from
the tradition of Forth. The dictionary of the Markab kernel gets initialized
with a core vocabulary containing definitions for these core words:

```
nop + - 1+ 1- * and inv or xor
<< >> >>>
= > < != 0= true false
r> >r rdrop r pc drop dup over swap
@ ! h@ h! w@ w!
iod ior iod iorh key emit
>a @a @a+     a+ a- a
>b @b @b+ !b+ b+ b- b >x x >y y
: ; var const opcode
if{ }if for{ break }for
```

Most of the core words are simple words that invoke a CPU instruction on one or
two arguments from the data or return stacks. The simple words do not run any
code when they are being compiled into a dictionary entry. "Compiling" a simple
word consists of adding the bytecode for that word's CPU instruction into the
dictionary entry that is being compiled.

The remaining core words are "immediate" words: `:`, `;`, `var`, `const`,
`opcode`, `if{`, `}if`, `for{`, and `}for`. Immediate words are used for
language features that require decisions and calculations at compile time. For
example, immediate words can do things like adding a dictionary entry or
calculating addresses for loops and branching.


## Markab file extension

The file extension for Markab source code is `.mkb`


## Sample source file

[sample.mkb](sample.mkb) is the sample code that I use to test syntax
highlighting.


## Emacs syntax highlighting

[markab-mode.el](markab-mode.el) is a simple emacs major mode to provide syntax
highlighting for the Markab source code in `.mkb` files.

Install Procedure:

1. Copy `markab-mode.el` into somewhere on your emacs load-path

2. Add this, or something similar, to your `.emacs`:
   ```
   (when (locate-library "markab-mode")
      (require 'markab-mode))
   ```


## Vim syntax highlighting

[vim/syntax/mkb.vim](vim/syntax/mkb.vim) and
[vim/ftdetect/mkb.vim](vim/ftdetect/mkb.vim) provide, respectively, vim syntax
highlighting and filetype detection for Markab `.mkb` source files.

Install Procedure:

1. Check the location of your syntax and filetype detect directories. On linux,
   they are probably `~/.vim/syntax` and `~/.vim/ftdetect`. You may need to
   create them. See: https://vimhelp.org/filetype.txt.html#new-filetype

2. If the directories do not already exist, create them. In bash:
   ```
   mkdir -p ~/.vim/syntax
   mkdir -p ~/.vim/ftdetect
   ```

3. Copy the `mkb.vim` files to the matching directories in `~/.vim/...`:
   ```
   cp vim/syntax/mkb.vim ~/.vim/syntax/
   cp vim/ftdetect/mkb.vim ~/.vim/syntax/
   ```

4. Open a .mkb file in vim. It should work. If not, you might need to do
   `:syntax enable` either manually or in your startup file.

See syntax highlighting docs at:
- https://vimhelp.org/usr_44.txt.html
- https://vimhelp.org/usr_43.txt.html#your-runtime-dir
- https://vimhelp.org/usr_43.txt.html#43.2  (adding a filetype)
