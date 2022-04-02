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
# Running main is special since it's effect is all about the return code
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

# These inference rules will assemble and link a .nasm input file.
# For example, if you do `make hello`, make will match and chain these rules
# as `obj/hello.o: hello.nasm`, then `hello: obj/hello.o` 
$(OBJECT_DIR)/%.o: %.nasm
	@mkdir -p $(OBJECT_DIR)
	@nasm -f elf64 -w+all -o $@ $<
%: $(OBJECT_DIR)/%.o
	@ld -s -o $@ $<

# The .PRECIOUS line prevents make from removing the intermediate .o files
# because that prints confusing `rm -f ...` lines after the output from %.run.
.PRECIOUS: $(OBJECT_DIR)/%.o
