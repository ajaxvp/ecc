EXE=ecc
OBJS=opt1.o opt4.o x86asm.o constexpr.o util.o tokenize.o allocate.o localize.o air.o preprocess.o type.o buffer.o const.o lex.o log.o traverse.o analyze.o parse.o map.o symbol.o syntax.o main.o vector.o
BUILD=$(addprefix build/, $(OBJS))

default: buildfolder libc/libc.a libecc/libecc.a $(EXE)

test: buildfolder libc/libc.a libecc/libecc.a $(EXE)
	cd test && $(MAKE) clean && $(MAKE) && ./test

buildfolder:
	mkdir -p build

libc/libc.a:
	cd libc && $(MAKE) clean && $(MAKE)

libecc/libecc.a:
	cd libecc && $(MAKE) clean && $(MAKE)

$(EXE): $(BUILD)
	gcc -g -o $(EXE) $^ $(LIBS)

build/%.o: src/%.c
	gcc -c -g -Wall -Werror=vla --std=c99 -o $@ $<

clean:
	rm -f $(EXE) $(BUILD)
	rm -rf build