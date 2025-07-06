OUT=libc.a
OBJS=putchar.o puts.o strlen.o stdlib.o
BUILD=$(addprefix build/, $(OBJS))

.PHONY: default clean

default: build $(OUT)

build:
	mkdir -p build

$(OUT): $(BUILD)
	ar r $(OUT) $^

build/%.o: src/%.s
	x86_64-linux-gnu-as -o $@ $<

build/%.o: src/%.c
	./../ecc -c -o $@ $<

clean:
	rm -f $(OUT) $(BUILD)
	rm -rf build