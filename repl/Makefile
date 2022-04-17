.POSIX:
.SUFFIXES:
.PHONY: help all clean
OBJECT_DIR=obj
TARGET=main

all: $(TARGET)
clean:
	@rm -f $(TARGET)
	@rm -rf $(OBJECT_DIR)

run: $(TARGET)
	@./$<

dump: $(TARGET)
	@objdump --visualize-jumps -d -M intel $<
	@readelf -x .data $<

$(OBJECT_DIR)/%.o: %.nasm
	@mkdir -p $(OBJECT_DIR)
	@nasm -f elf64 -w+all --reproducible -o $@ $<

# Enable `make foo` to create the program `foo` from `foo.c`
%: %.c
	$(CC) -std=c99 -Wall -o $@ $<

# Enable `make obj/bar.o` to create object file `obj/bar.o` from `bar.c`
$(OBJECT_DIR)/%.o: %.c
	$(CC) -std=c99 -Wall -o $@ $<
