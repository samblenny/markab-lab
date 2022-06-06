( Copyright © 2022 Sam Blenny)
( SPDX-License-Identifier: MIT)
( MarkabForth kernel: binary rom image to be compiled by bootstrap compiler)


( TODO: make this actually work)
( TODO: configure dictionary pointer for new definitions to go to rom file)

var DP       ( dictionary pointer)
var Current  ( head of dictionary)
var TIB      ( terminal input buffer)
var IBPtr    ( input buffer pointer, can be terminal or file)
var IBLen    ( total bytes available in input buffer)
var IN       ( current input buffer position: next byte to be read)

: ! ... ;
: @ ... ;
: + ... ;
: - ... ;
( lots more words for CPU instruction tokens)

: word   ... ;
: create ... does> ... ;
: does>  ... does> ... ;

: var   create ... does> ... ;
: const create ... does> ... ;
: :     create ... does> ... ;
: ;            ... does> ... ;

: ( ... ;

: inner ( inner interpreter) ;