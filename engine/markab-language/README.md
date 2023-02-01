<!-- Copyright (c) 2022 Sam Blenny -->
<!-- SPDX-License-Identifier: MIT -->

# Markab Language (Game Engine Version)

This page documents the Markab programming language, including:
- VM opcodes
- Kernel words
- VM error codes
- Keyword syntax highlighting for emacs and vim

Table of contents:
- [Markab Language Overview](#markab-language-overview)
- [Markab File Extension](#markab-file-extension)
- [Sample Source File](#sample-source-file)
- [Emacs Syntax Highlighting](#emacs-syntax-highlighting)
- [Vim Syntax Highlighting](#vim-syntax-highlighting)
- [VM Registers](#vm-registers)
- [VM Opcodes](#vm-opcodes)
- [Core Vocabulary](#core-vocabulary)
- [VM Error Codes](#vm-error-codes)


## Markab Language Overview

The Markab programming language is stack oriented, uses Reverse Polish Notation
(RPN), and has minimal syntax. Markab is Forth-like in its implementation. But,
Markab is not a Forth, in that it does not follow the official Forth standards.

These are instruction opcode mnemonics for the Markab VM (virtual machine):

```
NOP ADD SUB INC DEC MUL DIV MOD AND INV OR XOR
SLL SRL SRA
EQ GT LT NE ZE TRUE FALSE JMP JAL CALL RET HALT
BZ BFOR MTR RDROP R PC MTE DROP DUP OVER SWAP
U8 U16 I32 LB SB LH SH LW SW RESET
IOD IODH IORH KEY EMIT DOT DUMP IOLOAD TRON TROFF
MTA LBA LBAI      AINC ADEC A
MTB LBB LBBI SBBI BINC BDEC B
```

These are keywords of the Markab programming language core vocabulary:

```
nop + - 1+ 1- * / % and inv or xor
<< >> >>>
= > < != 0= true false call halt
>r rdrop r pc >err drop dup over swap
@ ! h@ h! w@ w! reset
.S .Sh iorh key emit . dump tron troff
>a @a @a+     a+ a- a
>b @b @b+ !b+ b+ b- b
: ; var const opcode
if{ }if for{ }for
```

This is a work in progress, so the keywords and opcodes will likely change.


## Markab File Extension

The file extension for Markab source code is `.mkb`


## Sample Source File

[sample.mkb](sample.mkb) is the sample code that I use to test syntax
highlighting.


## Emacs Syntax Highlighting

[markab-mode.el](markab-mode.el) is a simple emacs major mode to provide syntax
highlighting for the Markab source code in `.mkb` files.

Install Procedure:

1. Copy `markab-mode.el` into somewhere on your emacs load-path

2. Add this, or something similar, to your `.emacs`:
   ```
   (when (locate-library "markab-mode")
      (require 'markab-mode))
   ```


## Vim Syntax Highlighting

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
   cp vim/ftdetect/mkb.vim ~/.vim/ftdetect/
   ```

4. Open a .mkb file in vim. It should work. If not, you might need to do
   `:syntax enable` either manually or in your startup file.

See syntax highlighting docs at:
- https://vimhelp.org/usr_44.txt.html
- https://vimhelp.org/usr_43.txt.html#your-runtime-dir
- https://vimhelp.org/usr_43.txt.html#43.2  (adding a filetype)


## VM Registers

You may find these register descriptions helpful for understanding the stack
effects in the table of opcodes below.

| Name | Description |
|------|-------------|
| ERR  | Error code register (set by `MTE`, see markab_vm.py for error codes) |
| A    | Source address or scratch (for data-copying loops) |
| B    | Destination address or scratch (for data-copying loops) |
| T    | Top item on data stack |
| S    | Second item on data stack |
| R    | Top item on return stack (for call threading and loop counters) |
| PC   | Program Counter (address of current instruction) |

The Markab VM does not provide memory mapped access to the stacks nor fancy
stack manipulation instructions. If you want to follow along with stack changes
in Markab, use the VM's debug tracing features.

Markab uses shallow stacks in the style of Chuck Moore's stack CPU designs.
The data stack can hold at most 18 items. The return stack can hold at most 17
items. The Markab CPU is not intended for deep call nesting nor passing lots of
data on the stack. If you need to pass lots of data, use variables or make your
own data structure in the dictionary with `var` and `allot`.

Markab's stack cell size, and the registers, are all effectively 32-bits wide,
although they may actually be implemented by the VM in a wider data.


## VM Opcodes

The table below lists Markab VM opcode mnemonics in alphabetical order.

Math operations are effectively 32-bit signed integer math. Behind the scenes
in the VM implementation, there is some sign extension and bit masking to allow
for 64-bit or arbitrary precision math on the host system. But, from the
perspective of Markab code, the virtual CPU is doing 32-bit integer math. Also,
to save on code space, there are provisions to load and store literals as uint8
(U8, "byte"), uint16 (U16, "halfword"), or int32 (I32, "word") in the style of
the RISC-V RV32I instruction set.

Addressing uses a 16-bit (64 kB) address space. Jumps and calls mostly use
signed PC-relative address offsets so the code can be relocated. The `BFOR`
instruction is special in that it uses an 8-bit unsigned PC-relative offset.
`BFOR` is used to implement the `}for` word which ends `for{...}for` loops. So,
there is a lurking gotcha in that the maximum compiled code size inside of a
`for{ ... }for` loop must be less than 255 bytes. This should be no problem if
you factor your code into short words (highly recommended).

Notes on the table of opcodes below:

1. In the descriptions, I use comparison operators in the style of C. For
   example, `T == 0` means testing for equality between zero and the value in
   register `T`.

2. Markab is case sensitive. So, for example, `NOP` the opcode is not the same
   as `nop` the core vocabulary word.

3. Many opcodes mnemonic names are not defined as kernel words. Instead, those
   opcodes have equivalent name that are more readable. For example, the word
   `+` can be used to invoke the opcode `ADD`. In some cases, opcodes like
   `JMP`, `JAL`, `RET`, etc. are defined as constants that push a bytecode to
   the stack so they can be used in defining immediate compiling words (e.g.
   `if{`, `}if`, `for{`, and `}for`).

4. For opcodes that have a directly corresponding core vocabulary word, that
   word is listed in the "Word" column of the table below. The other opcodes
   only make sense as part of a compiled definition, so they do not have words
   defined to invoke them interactively.

5. For words implementing math operations, the operator names are mostly taken
   from C.

6. Comparison operations are not like C because I'm using weird old-school
   Forth-style truth values. True is -1 and false is 0. This method allows for
   using one set of opcodes, like `AND` and `OR` to handle both logical
   (boolean) and bitwise operations. Otherwise, I would need to provide having
   two sets of operators, like `&`, `|`, `&&`, and `||` in C.

| Opcode | Word | Description |
|--------|------|-------------|
| A      | a    | Push a copy of register A to the data stack |
| ADD    | +    | Add T to S, store result in S, drop T |
| ADEC   | a-   | Subtract 1 from register A |
| AINC   | a+   | Add 1 to register A |
| AND    | and  | Store bitwise AND of S with T into S, then drop T |
| B      | b    | Push a copy of register B to the data stack |
| BDEC   | b-   | Subtract 1 from register B |
| BFOR   |      | Decrement R and branch to start of for-loop if R > 0. Branch offset is read from instruction stream as 8-bit unsigned int to be subtracted from PC (branch direction is always backwards). |
| BINC   | b+   | Add 1 to register B |
| BZ     |      | Branch to PC-relative address (read from instruction stream) if T == 0, drop T. The branch address is PC-relative to allow for relocatable object code. |
| CALL   | call | Call to subroutine at address T, pushing old PC to return stack |
| DEC    | 1-   | Subtract 1 from T |
| DIV    | /    | Divide S by T (integer division), store quotient in S, drop T |
| DROP   | drop | Drop T, the top item of the data stack |
| DUP    | dup  | Push a copy of T |
| EQ     | =    | Evaluate S == T (true:-1, false:0), store result in S, drop T |
| FALSE  | false | Push 0 (false) to data stack |
| GT     | >    | Evaluate S > T (true:-1, false:0), store result in S, drop T |
| HALT   | halt | Set the halt flag to stop further instructions (used for `bye`) |
| I32    |      | Read int32 word (4 bytes) signed literal from instruction stream, push as T |
| INC    | 1+   | Add 1 to T |
| INV    | inv  | Invert the bits of T (ones' complement negation) |
| IOD    | .S   | Debug dump data stack in decimal format |
| IODH   | .Sh  | Debug dump data stack in hexadecimal format |
| DOT    | .    | Print T to standard output, then drop T |
| DUMP   | dump | Hexdump S bytes of memory starting from address T, then drop S and T |
| EMIT   | emit | Buffer the low byte of T for stdout. |
| KEY    | key  | Push the next byte from Standard Input to the data stack. Stack effect depends on whether a byte was available. When byte is available, push the data byte, then push true (-1): result is T=-1 (true), S=data-byte. When no data is available, push false (0): result is T=0 (false). |
| IORH   | iorh | Debug dump return stack in hexadecimal format |
| JAL    |      | Jump to 16-bit address (from instruction stream) after pushing old value of PC to return stack. The jump address is PC-relative to allow for relocatable object code. |
| JMP    |      | Jump to 16-bit address (from instruction stream). The jump address is PC-relative to allow for relocatable object code. |
| LB     | @    | Load a uint8 (1 byte) from memory address T, saving result in T |
| LBA    | @a   | Load byte from memory using address in register A |
| LBAI   | @a+  | Load byte from memory using address in register A, then increment A |
| LBB    | @b   | Load byte from memory using address in register B |
| LBBI   | @b+  | Load byte from memory using address in register B, then increment B |
| LH     | h@   | Load halfword (2 bytes, zero-extended) from memory address T, into T |
| LT     | <    | Evaluate S < T (true:-1, false:0), store result in S, drop T |
| LW     | w@   | Load a signed int32 (word = 4 bytes) from memory address T, into T |
| MOD    | %    | Divide S by T (integer division), store remainder in S, drop T |
| MTA    | >a   | Move top of data stack (T) to register A |
| MTB    | >b   | Move top of data stack (T) to register B |
| MTE    | >err | Move top of data stack (T) to ERR register (raise an error) |
| MTR    | >r   | Move top of data stack (T) to top of return stack (R) |
| MUL    | *    | Multiply S by T, store result in S, drop T |
| NE     | !=   | Evaluate S != T (true:-1, false:0), store result in S, drop T |
| NOP    | nop  | Spend one clock cycle doing nothing |
| OR     | or   | Store bitwise OR of S with T into S, then drop T |
| OVER   | over | Push a copy of S |
| PC     | pc   | Push a copy of the Program Counter (PC) to the data stack |
| R      | r    | Push a copy of top of Return stack (R) to the data stack |
| RDROP  | rdrop | Drop R in the manner needed when exiting from a counted loop |
| RESET  | reset | Reset the data stack, return stack, error code, and input buffer |
| RET    |      | Return from subroutine, taking address from return stack |
| SB     | !    | Store low byte of S (uint8) at memory address T |
| SBBI   | !b+  | Store low byte of T byte to address in register B, then increment B |
| SH     | h!   | Store low 2 bytes from S (uint16) at memory address T |
| SLL    | <<   | Shift S left by T, store result in S, drop T |
| SRA    | >>>  | Signed (arithmetic) shift S right by T, store result in S, drop T |
| SRL    | >>   | Unsigned (logic) shift S right by T, store result in S, drop T |
| SUB    | -    | Subtract T from S, store result in S, drop T |
| SW     | w!   | Store word (4 bytes) from S as signed int32 at memory address T |
| SWAP   | swap | Swap S with T |
| TROFF  | troff | Disable debug tracing (also see `DEBUG` global var in `markab_vm.py`) |
| TRON   | tron | Enable debug tracing (also see `DEBUG` global var in `markab_vm.py`) |
| TRUE   | true | Push -1 (true) to data stack |
| U16    |      | Read uint16 halfword (2 bytes) literal from instruction stream, zero-extend it, push as T |
| U8     |      | Read uint8 byte literal from instruction stream, zero-extend it, push as T |
| XOR    | xor  | Store bitwise XOR of S with T into S, then drop T |
| ZE     | 0=   | Evaluate 0 == T (true:-1, false:0), store result in T |


# Core Vocabulary

Many of the core vocabulary words are just convenient names for Markab VM
opcodes. For descriptions of core vocabulary words that correspond to opcodes,
refer to the [VM Opcodes](#vm-opcodes) section above.

The Markab kernel also defines additional words that provide higher level
constructs such as control flow and memory allocation.

The Markab language uses a Forth-style dictionary which I sometimes refer to in
comments as "heap", following the old Forth usage of that term. Note that this
usage of "heap" is very different from the meanings of "stack" and "heap" in
the context of using stack allocation or `malloc()` in C code.

The Markab kernel's dictionary can have four types of entries:

1. Subroutine: Parameter field contains compiled code for a subroutine.
   Invoking the word results in an action of: kernel calls the subroutine.

2. Opcode: Parameter field contains the bytecode for a VM opcode. Invoking
   the word results in an action of: kernel directly runs the opcode.

3. Variable: Parameter field contains space for storing data. Invoking the
   word results in an action of: kernel pushes the parameter field's
   address onto the data stack.

4. Constant: Parameter field contains an integer constant. Invoking the
   word results in an action of: kernel pushes the value of the constant
   onto the data stack.

The Markab kernel implements a bunch of words, many of which are internal
implementation details that I don't recommend relying upon. The table below
describes kernel words that provide important control flow, compilation, IO,
and memory allocation constructs that help to make Markab extensible and
usable. I don't expect these words to change much:

| Word   | Description |
|--------|-------------|
| DP     | Dictionary Pointer: variable holding a pointer to the first free byte after the end of the dictionary. Many kernel words use this as a scratch buffer for string operations. Don't use DP directly, instead invoke `here`. |
| BASE   | Variable holding the number base (10 or 16). Set this with `hex` or `decimal`. |
| :      | Begin compiling new word into dictionary, reading name from input stream |
| ;      | Finish compiling a word, *OR*, inside of `if{...}if` or `for{...}for`, conditionally return from a word |
| var    | Compile space for a 32-bit variable into dictionary, reading name from input stream. Size can be increased with `allot`. Usage: `var foo` |
| const  | Compile value from T into the dictionary, reading name from stdin, then drop T. Usage: `12 const dozen` |
| create | Compile a name from input stream into the dictionary (for making your own data types). Usage: `create foo ...` |
| allot  | Add T to the dictionary pointer (DP), drop T. This lets you allocate extra space for the word most recently added to the dictionary. For example, you could use `allot` to make an array. Usage: `var array 10 4 * allot` |
| here   | Fetch value of DP and push that to T. This is for finding the next free byte after the end of the dictionary. |
| if{    | Immediate mode compiling word to begin a `if{...}if` conditional block during a :-definition. At runtime, this will evaluate the true/false value of T, drop T, and run the code inside the conditional block only if T was non-zero (a set of values that includes the "true" value of -1). When T is false (0) at runtime, `if{` will branch to the instruction following its matching `}if`. Note that you can implement if-else style semantics by ending a conditional block with `;`. For example: `: foo if{ if-block ; }if pseudo-else-block ;`. |
| }if    | Immediate mode compiling word to end a conditional block started by `if{` (see description of `if{`) |
| for{   | Immediate mode compiling word to begin a `for{...}for` counted loop during a `:`-definition. At runtime, `for{` moves the value of T to R and uses R for the loop counter. The loop block always runs at least once. At the end of the loop, `}for` will decrement R and branch back to the instruction after `for{` if R is greater than 0. This is designed so you can do `3 for{ ... }for` to have the loop body run 3 times, `4 for{ ... }for` to have the loop body run 4 times, and so on. In other words, the loop count value from T specifies the maximum number of iterations. You can break out of a for loop earlier than that using the idiom `for{ ... if{ rdrop ; }if }for`. |
| }for   | Immediate mode compiling word to end a `for{...}for` counted loop (see `for{` above) |
| decimal | Set number base to 10. This affects number parsing and formating for words like `var`, `const`, `.`. |
| hex    | Set number base to 16. This affects number parsing and formating for words like `var`, `const`, `.`. |
| "      | Immediate mode word to compile string into dictionary, reads from input stream until next instance of `"`. The string bytes just go at the end of the last dictionary entry but there isn't a named header. The address of the first byte of the string gets pushed into T. Strings are stored with a length-byte prefix, so " hello" would become 5, 'h', 'e', 'l', 'l', 'o', and the pointer in T would point to the address of the `5` byte. Maximum string length is 255 bytes. |
| print  | Print a string that begins at the address stored in T. Usage: `" hello" print` |
| (      | Immediate mode word to compile (skip) a comment, reads from input stream until next instance of `)` |
| bye    | Halt the VM, causing the process for markab_vm.py to exit |

Example of using `if{...}if` conditional block:
```
( Note how the `;` right before `}if` does an early return so the rest of)
(   the word acts like what other languges would do with an else-block)
: check ( bool -- )  if{ " true" print ; }if " false" print ;
  OK
0 check
false  OK
-1 check
true  OK
```

Examples of `for{...}for` counted loop blocks:
```
( This is printing the value of R, the loop counter)
: zero-indexed-down 1- for{ r . ( <- print loop counter) }for ;
  OK
3 zero-indexed-down
 2 1 0  OK
0 zero-indexed-down  ( note how loop always runs at least once)
 -1  OK

( This is printing register A which is initialized to 0)
: zero-index-up 0 >a 1- for{ a . a+ }for ;
  OK
3 zero-index-up
 0 1 2  OK
0 zero-index-up
 0  OK

( This uses the A and B registers to reverse a string)
: reverse ( str-ptr -- )
  >a      ( store string pointer address in A)
  @a      ( fetch string's length byte via pointer in A)
  a + >b  ( calculate pointer to end of string, store that address in B)
  @a 1-   ( prepare another copy of length byte as loop counter)
  for{    ( loop for lengthByte iterations)
    @b emit  ( fetch string byte through pointer in B, then print it)
    b-       ( decrement string pointer)
  }for    ( end of loop)
;
  OK
" !dlroW ,olleH" const greet  ( store a string in dictionary as `greet`)
  OK
greet reverse
Hello, World!  OK
```


## VM Error Codes

The Markab VM uses integer codes to indicate error conditions, and the kernel's
error handler currently just prints them in unfriendly messages like `ERR 2`.
This is what the error codes mean (taken from markab_vm.py):

| Code | markab_vm.py name | Meaning |
|------|-------------------|---------|
| 1  | ERR_D_OVER          | Data stack overflow |
| 2  | ERR_D_UNDER         | Data stack underflow |
| 3  | ERR_BAD_ADDRESS     | Expected vaild address but got something else |
| 4  | ERR_BOOT_OVERFLOW   | ROM image is too big to fit in the heap |
| 5  | ERR_BAD_INSTRUCTION | Expected an opcode but got something else |
| 6  | ERR_R_OVER          | Return stack overflow |
| 7  | ERR_R_UNDER         | Return stack underflow |
| 8  | ERR_MAX_CYCLES      | Call ran for too many clock cycles |
| 9  | ERR_FILEPATH        | Filepath failed VM sandbox permission check |
| 10 | ERR_FILE_NOT_FOUND  | Unable to open specified filepath |
| 11 | ERR_UNKNOWN         | Outer interpreter encountered an unknown word |
| 12 | ERR_NEST            | Compiler encountered unbalanced }if or }for |
| 13 | ERR_IOLOAD_DEPTH    | Too many levels of nested `load ...` calls |
| 14 | ERR_BAD_PC_ADDR     | Bad program counter value: address not in heap |
| 15 | ERR_IOLOAD_FAIL     | Error while loading a file |
| 16 | ERR_NO_OPEN_FILE    | Requested operation needs open file from FOPEN |
| 17 | ERR_OPEN_FILE       | Attempt to use FOPEN when file is already open |
| 18 | ERR_FILE_IO_FAIL    | Catchall for errors from host OS file IO API |
| 19 | ERR_UTF8            | Error decoding UTF-8 string |
