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
    "NOP" "ADD" "SUB" "MUL" "AND" "INV" "OR" "XOR" "SLL" "SRL" "SRA"
    "EQ" "GT" "LT" "NE" "ZE" "JMP" "JAL" "RET"
    "BZ" "DRBLT" "MRT" "MTR" "RDROP" "DROP" "DUP" "OVER" "SWAP"
    "U8" "U16" "I32" "LB" "SB" "LH" "SH" "LW" "SW" "RESET" "ECALL"

    ;; ECALL Codes
    "E_DS" "E_DSH" "E_RS" "E_RSH" "E_PC" "E_READ" "E_WRITE"

    ;; Core Words
    "nop" "+" "-" "*" "&" "~" "|" "^" "<<" ">>" ">>>"
    "=" ">" "<" "!=" "0="
    ":" ";" "var" "const"
    "r>" ">r" "rdrop" "drop" "dup" "over" "swap"
    "b@" "b!" "h@" "h!" "w@" "w!"
    "if{" "}if" "for{" "}for" "ASM{" "}ASM"))

(defconst markab-comments '(("( " . ")")))
(defconst markab-fontlocks '())
(defconst markab-auto-modes '("\\.mkb\\'"))
(defconst markab-functions '())

;; Add single quote as string delimiter
(modify-syntax-entry ?' "\"")

;; Put these characters in "word constituents" class to enable use in keywords
(modify-syntax-entry ?~ "\w")
(modify-syntax-entry ?^ "\w")
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
(modify-syntax-entry ?& "\w")
(modify-syntax-entry ?| "\w")
(modify-syntax-entry ?< "\w")
(modify-syntax-entry ?> "\w")
(modify-syntax-entry ?= "\w")
(modify-syntax-entry ?[ "\w")
(modify-syntax-entry ?] "\w")
(modify-syntax-entry ?. "\w")

;;;###autoload
(define-generic-mode 'markab-mode markab-comments markab-keywords
                     markab-fontlocks markab-auto-modes
                     markab-functions)

(provide 'markab-mode)
