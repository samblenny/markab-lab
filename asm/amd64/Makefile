.POSIX:
.SUFFIXES:
.PHONY: all clean
.SILENT: run

all: main.out hello.out hellothread.out
clean:
	rm -f *.out *.o

run-main: main.out
	@./main.out || echo "main exit: $$?"
run-hello: hello.out
	@./hello.out
run-hellothread: hellothread.out
	@./hellothread.out

# objdump'ing the .out (linked) file rather than the .o makes it possible to
# see pointer values into the relocated .data segment. In the .o, pointers
# show up as `QWORD PTR ds:0x0`, which is not helpful.
dump-main: main.out
	@objdump -d -M intel $<
	@readelf -x .data $<
dump-hello: hello.out
	@objdump -d -M intel $<
	@readelf -x .data $<
dump-hellothread: hellothread.out
	@objdump -d -M intel $<
	@readelf -x .data $<

elf-main: main.out
	readelf -a $<
elf-hello: hello.out
	readelf -a $<
elf-hellothread: hellothread.out
	readelf -a $<

# These inference rules will assemble and link a .nasm input file.
# The .PRECIOUS line prevents make from removing the intermediate .o files.
.SUFFIXES: .nasm .o .out
.PRECIOUS: %.o
.nasm.o:
	nasm -f elf64 -w+all -o $@ $<
.o.out:
	ld -s -o $@ $<
