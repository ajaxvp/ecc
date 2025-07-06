EXE=ecc
OBJS=opt1.o opt4.o x86asm.o constexpr.o util.o tokenize.o allocate.o localize.o air.o preprocess.o type.o buffer.o const.o lex.o log.o traverse.o analyze.o parse.o map.o symbol.o syntax.o main.o vector.o
BUILD=$(addprefix build/, $(OBJS))

.PHONY: default test buildfolder clean

default: buildfolder $(EXE) libecc/libecc.a libc/libc.a

test: buildfolder $(EXE) libecc/libecc.a libc/libc.a
	cd test && $(MAKE) && ./test

buildfolder:
	mkdir -p build

libc/libc.a:
	cd libc && $(MAKE)

libecc/libecc.a:
	cd libecc && $(MAKE)

$(EXE): $(BUILD)
	gcc -g -o $(EXE) $^ $(LIBS)

build/%.o: src/%.c
	gcc -c -g -Wall -Werror=vla --std=c99 -o $@ $<

clean:
	cd test && $(MAKE) clean
	cd libc && $(MAKE) clean
	cd libecc && $(MAKE) clean
	rm -f $(EXE) $(BUILD)
	rm -rf build