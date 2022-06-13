<!-- Copyright (c) 2022 Sam Blenny -->
<!-- SPDX-License-Identifier: MIT -->

# Markab Language

This page documents the Markab programming language. Or, more accurately,
that's the plan, which is still in the early stages of implementation.


## Markab file extension

The file extension for Markab source code is `.mkb`


## Sample source file

[sample.mkb](sample.mkb) is the sample code that I use to test syntax
highlighting.


## Emacs syntax highlighting

[markab-mode.el](markab-mode.el) is a simple emacs major mode to provide syntax
highlighting for the Markab source code in `.mkb` files.

Install Procedure:

1. Copy `markab-mode.el` to your into somewhere on your emacs load-path

2. Add something similar to this to your `.emacs`:
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
