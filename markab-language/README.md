<!-- Copyright (c) 2022 Sam Blenny -->
<!-- SPDX-License-Identifier: MIT -->

# Markab Language

This page documents the Markab programming language. Or, more accurately,
that's the plan, which is still in the early stages of implementation.


## Language Specification

The Markab programming language is stack oriented, uses Reverse Polish Notation
(RPN), and has minimal syntax. Markab is Forth-like in its implementation, but
Markab is not a Forth.

Similarities between Markab and Forth include:
- Two stacks: data stack for arguments and return stack for subroutine calls
- Extensible dictionary for defining a vocabulary of words (subroutines)
- Words for math and stack operations (+, -, dup, over, swap, ...)
- Words for some text IO operations (key, emit, ...)
- `: ... ;` colon definitions
- `( ...)` comments
- Compiler uses regular and immediate words and data stack for jump addresses
- Outer interpreter to parse input and look up words in dictionary
- Inner interpreter to run words as direct-threaded object code

Aside from those things, Markab is generally not like Forth, and it does not
comply with any of the Forth standards.

These are instruction opcode mnemonics for the Markab VM (virtual machine):

```
NOP ADD SUB INC DEC MUL DIV MOD AND INV OR XOR
SLL SRL SRA
EQ GT LT NE ZE TRUE FALSE JMP JAL CALL RET HALT
BZ BFOR MTR RDROP R PC ERR MTE DROP DUP OVER SWAP
U8 U16 I32 LB SB LH SH LW SW RESET
IOD IODH IORH IOKEY IOEMIT IODOT IODUMP IOLOAD IOSAVE TRON TROFF
MTA LBA LBAI      AINC ADEC A
MTB LBB LBBI SBBI BINC BDEC B
```

These are keywords of the Markab programming language core vocabulary:

```
nop + - 1+ 1- * / % and inv or xor
<< >> >>>
= > < != 0= true false call halt
>r rdrop r pc err >err drop dup over swap
@ ! h@ h! w@ w! reset
iod iod iorh key emit . dump load save tron troff
>a @a @a+     a+ a- a
>b @b @b+ !b+ b+ b- b
: ; var const opcode
if{ }if for{ }for
```

The keywords and opcodes are still in a stage of evolving changes as I work to
implement and debug the VM and compiler. I plan to document this better once
things have stabilized. For now, the authoritative references are the VM source
code and the tests:
- [../repl2/markab_vm.py](../repl2/markab_vm.py)
- [../repl2/test](../repl2/test)


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
