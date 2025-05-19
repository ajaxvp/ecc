EXE=ecc
OBJS=constexpr.o util.o tokenize.o iropt.o x86gen.o allocate.o linearize.o preprocess.o type.o buffer.o const.o lex.o log.o traverse.o analyze.o parse.o set.o symbol.o syntax.o main.o vector.o
BUILD=$(addprefix build/, $(OBJS))

default: buildfolder $(EXE)

test: buildfolder $(EXE)
	cd test && $(MAKE) clean && $(MAKE) && ./test

buildfolder:
	mkdir -p build

$(EXE): $(BUILD)
		gcc -g -o $(EXE) $^ $(LIBS)

build/%.o: src/%.c
		gcc -c -g -Wall -Werror=vla --std=c99 -o $@ $<

clean:
		rm -f $(EXE) $(BUILD)
		rm -rf build