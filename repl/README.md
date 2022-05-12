# Markab Forth REPL

This is an interactive Forth system that I'm writing in a combination of C99
and amd64 (nasm) assembly language. For now, the UI is limited to a command
line shell, but I hope to eventually add sound and graphics capability.

This is a work in progress.


## Usage

1. Clone the repo on linux system with a 64-bit Intel or AMD CPU. I'm using
   Debian 11 (Bullseye), but other distros will probably work fine.

2. Make sure you have `make`, `clang`, and `nasm` installed.

3. Run tests: `make test`

4. Run the shell: `make run`


## Example Session

```
$ make run
nasm -f elf64 -w+all --reproducible -o obj/libmarkab.o libmarkab.nasm
clang -O2 -Wl,-export-dynamic -std=c99 -Wall -o main main.c obj/libmarkab.o
Markab v0.0.1
type 'bye' or ^C to exit
 __  __          _        _
|  \/  |__ _ _ _| |____ _| |__
| |\/| / _` | '_| / / _` | '_ \
|_|  |_\__,_|_| |_\_\__,_|_.__/

  1 2 3
  7  OK
cr ." Hello World!"
Hello World!  OK
( this is a comment)  OK
( do some math) 1 1 + . 2  OK
( define a new word...)  OK
: 1+ 1 + ;  OK
3 1+ . 4  OK
( the `.` word pops the top item of the stack and prints it)  OK
bye  OK
```

The banner and "1 2 3... 7  OK" stuff comes from the load screen, which gets
included in the binary at compile time from [screen00.fs](screen00.fs).
