EXE=test
TEST_OBJS=parse.o
OBJS=testutils.o main.o test.o
PRGM_OBJS=util.o buffer.o const.o emit.o lex.o log.o parse.o set.o symbol.o syntax.o vector.o
TEST_BUILD=$(addprefix build/, $(TEST_OBJS))
BUILD=$(addprefix build/, $(OBJS))
PRGM_BUILD=$(addprefix ../build/, $(PRGM_OBJS))
TEST_SOURCE=$(addprefix src/, $(TEST_OBJS:.o=.c))

default: buildfolder gen $(EXE)

buildfolder:
	mkdir -p build

gen:
	python3 gen.py $(TEST_SOURCE)

$(EXE): $(TEST_BUILD) $(PRGM_BUILD) $(BUILD)
		gcc -g -o $(EXE) $^ $(LIBS)

build/%.o: src/%.c
		gcc -c -g -Wall -D_POSIX_SOURCE --std=c11 -o $@ $<

clean:
		rm -f $(EXE) $(TEST_BUILD) $(BUILD)
		rm -rf build