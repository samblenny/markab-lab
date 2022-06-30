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
    "EQ" "GT" "LT" "NE" "ZE" "TRUE" "FALSE" "JMP" "JAL" "CALL" "RET"
    "BZ" "BFOR" "MRT" "MTR" "RDROP" "R" "PC" "ERR" "DROP" "DUP" "OVER" "SWAP"
    "U8" "U16" "I32" "LB" "SB" "LH" "SH" "LW" "SW" "RESET" "FENCE" "CLERR"
    "IOD" "IOR" "IODH" "IORH" "IOKEY" "IOEMIT" "IODOT" "IODUMP" "TRON" "TROFF"
    "MTA" "LBA" "LBAI"        "AINC" "ADEC" "A"
    "MTB" "LBB" "LBBI" "SBBI" "BINC" "BDEC" "B"

    ;; Core Words
    "nop" "+" "-" "1+" "1-" "*" "and" "inv" "or" "xor"
    "<<" ">>" ">>>"
    "=" ">" "<" "!=" "0=" "true" "false" "call"
    "r>" ">r" "rdrop" "drop" "r" "pc" "err" "dup" "over" "swap"
    "@" "!" "h@" "h!" "w@" "w!" "reset" "fence" "clerr"
    "iod" "ior" "iodh" "iorh" "key" "emit" "." "dump" "tron" "troff"
    ">a" "@a" "@a+"       "a+" "a-" "a"
    ">b" "@b" "@b+" "!b+" "b+" "b-" "b"
    ":" ";" "var" "const" "opcode"
    "if{" "}if" "for{" "}for"))

(defconst markab-comments '(("( " . ")")))
(defconst markab-fontlocks '())
(defconst markab-auto-modes '("\\.mkb\\'"))
(defconst markab-functions '())

;; Put these characters in "word constituents" class to enable use in keywords
(modify-syntax-entry ?! "\w")
(modify-syntax-entry ?. "\w")
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
