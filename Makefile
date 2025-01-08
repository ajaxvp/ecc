EXE=cc
OBJS=buffer.o const.o emit.o lex.o log.o parse.o set.o symbol.o syntax.o test.o vector.o
BUILD=$(addprefix build/, $(OBJS))

default: buildfolder $(EXE)

buildfolder:
	mkdir build

$(EXE): $(BUILD)
		gcc -g -o $(EXE) $^ $(LIBS)

build/%.o: src/%.c
		gcc -c -g -Wall --std=c11 -o $@ $<

clean:
		rm -f $(EXE) $(BUILD)
		rm -rf build