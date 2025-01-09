EXE=test
OBJS=testutils.o main.o parse.o test.o
PRGM_OBJS = util.o buffer.o const.o emit.o lex.o log.o parse.o set.o symbol.o syntax.o vector.o
BUILD=$(addprefix build/, $(OBJS))
PRGM_BUILD=$(addprefix ../build/, $(PRGM_OBJS))

default: buildfolder $(EXE)

buildfolder:
	mkdir -p build

$(EXE): $(PRGM_BUILD) $(BUILD)
		gcc -g -o $(EXE) $^ $(LIBS)

build/%.o: src/%.c
		gcc -c -g -Wall -D_POSIX_SOURCE --std=c11 -o $@ $<

clean:
		rm -f $(EXE) $(BUILD)
		rm -rf build