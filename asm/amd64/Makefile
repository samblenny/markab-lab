# This Makefile is intended to easily build, run, and inspect lots of little
# experiments without having to constantly make edits for adding new rules. To
# understand what's going on here, you need to become familiar with the
# concepts of "automatic variables", "implicit rules" and "pattern rules",
# which are described here:
# - https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
# - https://www.gnu.org/software/make/manual/html_node/Implicit-Rules.html
# - https://www.gnu.org/software/make/manual/html_node/Pattern-Rules.html
.POSIX:
.SUFFIXES:
.PHONY: help all clean
OBJECT_DIR=obj
TARGETS=main hello hellothread repl

help:
	@echo "you need to specify a target"
all: $(TARGETS)
clean:
	@rm -f $(TARGETS)
	@rm -rf $(OBJECT_DIR)

# To invoke this, do `make main.run`
# Running main is special since its effect is all about the return code
main.run: main
	@./$< || echo "$< exit: $$?"

# Enable `make hello.run`, `make repl.run`, or generally `make $WHATEVER.run`
%.run: %
	@./$<

# Enable `make hello.dump`, `make repl.dump`, etc. for disassembly.
%.dump: %
	@objdump -d -M intel $<
	@readelf -x .data $<

# Enable `make hello.elf`, and so on for a full ELF header listing.
%.elf: %
	readelf -a $<

# These pattern rules define an implicit rule to automatically assemble and
# link a .nasm input file. For example, if you do `make hello`, make will match
# and chain these rules as `obj/hello.o: hello.nasm`, then `hello: obj/hello.o`,
# as if there were a rule for `hello: hello.nasm` (the implicit rule).
$(OBJECT_DIR)/%.o: %.nasm
	@mkdir -p $(OBJECT_DIR)
	@nasm -f elf64 -w+all -o $@ $<
%: $(OBJECT_DIR)/%.o
	@ld -s -o $@ $<

# The .PRECIOUS line prevents make from removing the intermediate .o files
# because that prints confusing `rm -f ...` lines after the output from %.run.
.PRECIOUS: $(OBJECT_DIR)/%.o
