# This Makefile is intended to easily build, run, and inspect lots of little
# experiments without having to constantly make edits for adding new rules. To
# understand what's going on here, you need to become familiar with the
# concepts of "automatic variables" and "pattern rules", which are described
# here:
# - https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
# - https://www.gnu.org/software/make/manual/html_node/Pattern-Rules.html
.POSIX:
.SUFFIXES:
.PHONY: help all clean
OBJECT_DIR=obj
TARGETS=main hello hellothread repl stacktest stacktest2 string1 token1 token2\
 token3

help:
	@echo "you need to specify a target"
all: $(TARGETS)
clean:
	@rm -f $(TARGETS)
	@rm -rf $(OBJECT_DIR)

# Enable `make hello.run`, `make repl.run`, or generally `make $WHATEVER.run`.
# The `%` prerequisite at the end of the rule will build the target program
# using the rule defined below for `%: %.nasm`.
%.run: %
	@./$<

# Running main is special since its effect is all about the return code.
# To invoke this, do `make main.run`
main.run: main
	@./$< || echo "$< exit: $$?"

# Enable `make hello.dump`, `make repl.dump`, etc. for disassembly.
%.dump: %
	@objdump --visualize-jumps -d -M intel $<
	@readelf -x .data $<

# Enable `make hello.elf`, and so on for a full ELF header listing.
%.elf: %
	readelf -a $<

# Enable `make hello` to create the program `hello` from `hello.nasm`.
# NOTE: `ld --export-dynamic ...` is good for disassembly with nice labels,
#       but it might be better to use `ld -s ...` if building for distro.
%: %.nasm
	@mkdir -p $(OBJECT_DIR)
	@nasm -f elf64 -w+all --reproducible -o $(OBJECT_DIR)/$@.o $<
	@ld --export-dynamic -o $@ $(OBJECT_DIR)/$@.o
