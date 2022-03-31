.POSIX:
.SUFFIXES:
.PHONY: all clean
.SILENT: run

all: main.out hello.out
clean:
	rm -f *.out *.o

run-main: main.out
	@./main.out || echo "main exit: $$?"
run-hello: hello.out
	@./hello.out

dump-main: main.o
	objdump -d -M intel $<
dump-hello: hello.o
	objdump -d -M intel $<

elf-main: main.out
	readelf -a $<
elf-hello: hello.out
	readelf -a $<

# These inference rules will assemble and link a .nasm input file.
# The .PRECIOUS line prevents make from removing the intermediate .o files.
.SUFFIXES: .nasm .o .out
.PRECIOUS: %.o
.nasm.o:
	nasm -f elf64 -w+all -o $@ $<
.o.out:
	ld -s -o $@ $<
