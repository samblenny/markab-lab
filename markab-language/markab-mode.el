;;; markab-mode.el --- emacs mode for editing Markab code

;; Copyright (c) 2022 Sam Blenny
;; SPDX-License-Identifier: MIT
;;
;; To use markab-mode for syntax highlighting of Markab source code in emacs,
;; copy markab-mode.el into your emacs load-path (perhaps in ~/.emacs.d/...),
;; then add something similar to this to your .emacs config file:
;;
;; (when (locate-library "markab-mode")
;;   (require 'markab-mode))

(defconst markab-keywords
  '(;; VM Opcodes
    "NOP" "ADD" "SUB" "INC" "DEC" "MUL" "AND" "INV" "OR" "XOR"
    "SLL" "SRL" "SRA"
    "EQ" "GT" "LT" "NE" "ZE" "TRUE" "FALSE" "JMP" "JAL" "RET"
    "BZ" "DRBLT" "MRT" "MTR" "RDROP" "R" "PC" "DROP" "DUP" "OVER" "SWAP"
    "U8" "U16" "I32" "LB" "SB" "LH" "SH" "LW" "SW" "RESET"
    "IOD" "IOR" "IODH" "IORH" "IOKEY" "IOEMIT"
    "MTA" "LBA" "LBAI"        "AINC" "ADEC" "A"
    "MTB" "LBB" "LBBI" "SBBI" "BINC" "BDEC" "B" "MTX" "X" "MTY" "Y"

    ;; Core Words
    "nop" "+" "-" "1+" "1-" "*" "and" "inv" "or" "xor"
    "<<" ">>" ">>>"
    "=" ">" "<" "!=" "0=" "true" "false"
    "r>" ">r" "rdrop" "drop" "r" "pc" "dup" "over" "swap"
    "@" "!" "h@" "h!" "w@" "w!"
    "iod" "ior" "iodh" "iorh" "key" "emit"
    ">a" "@a" "@a+"       "a+" "a-" "a"
    ">b" "@b" "@b+" "!b+" "b+" "b-" "b" ">x" "x" ">y" "y"
    ":" ";" "var" "const"
    "if{" "}if" "for{" "break" "}for" "ASM{" "}ASM"))

(defconst markab-comments '(("( " . ")")))
(defconst markab-fontlocks '())
(defconst markab-auto-modes '("\\.mkb\\'"))
(defconst markab-functions '())

;; Put these characters in "word constituents" class to enable use in keywords
(modify-syntax-entry ?! "\w")
(modify-syntax-entry ?: "\w")
(modify-syntax-entry ?; "\w")
(modify-syntax-entry ?@ "\w")
(modify-syntax-entry ?{ "\w")
(modify-syntax-entry ?} "\w")
(modify-syntax-entry ?+ "\w")
(modify-syntax-entry ?- "\w")
(modify-syntax-entry ?* "\w")
(modify-syntax-entry ?/ "\w")
(modify-syntax-entry ?< "\w")
(modify-syntax-entry ?> "\w")
(modify-syntax-entry ?= "\w")
(modify-syntax-entry ?' "\w")

;;;###autoload
(define-generic-mode 'markab-mode markab-comments markab-keywords
                     markab-fontlocks markab-auto-modes
                     markab-functions)

(provide 'markab-mode)
