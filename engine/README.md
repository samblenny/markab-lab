<!-- Copyright (c) 2023 Sam Blenny -->
<!-- SPDX-License-Identifier: MIT -->

# Markab Game Engine

You're looking at experimental work in progress that I'm using to explore ideas
about a game engine with Markab scripting.

This is like a laboratory. Things you encounter here may change suddenly, or
they may be stable for a long time. It just depends on how the work unfolds.


## Running Game Engine Front End Demo

The markab game engine is structured as a back-end library written in C, and a
front-end GUI written in Javascript and GLSL.

Keeping in mind that this is a work in progress, if you want to see what the
front-end looks like, the files are in [./www/](www). That stuff is also hosted
on GitHub Pages at:

https://samblenny.github.io/markab-lab/engine/www/

If you want to experiment with the code locally, you can do something like
this:

```
$ cd www
$ ./webserver.rb &
$ cd ..
$ # load the page in a browser at http://localhost:8000
$ # if you want, edit files. If you want to recompile the wasm module, do...
$ make wasm
$ # when you're done and want to stop the webserver script...
$ fg    # then type control-C
```

Note that the webserver.rb script depends on ruby2.7+ and WEBrick, which is
part of the ruby standard library. The main point of the script is to serve the
wasm module with the proper mime type to make chrome or firefox happy. Trying
to load the page by opening the www/index.html as a file is unlikely to work.

To compile the wasm module, you need a version of the clang (LLVM) C compiler
that supports the wasm32 target. I use clang version 11 on Debian Bullseye, and
it works great. To check if your version of clang supports wasm32, you can do
this:

```
$ clang --version
Debian clang version 11.0.1-2
...
$ clang --print-targets | grep wasm
    wasm32     - WebAssembly 32-bit
    wasm64     - WebAssembly 64-bit
```


## Running Back-End Demos and Tests

The back-end library has a markabscript compiler and VM, among other things. To
try out the back-end on macOS and Debian, and potentially other POSIX systems:

```
$ make         # build the CLI demo
...
$ make run     # build and run the CLI demo
...
$ make test    # build and run the tests
...
$ make clean   # remove all the build files
```
