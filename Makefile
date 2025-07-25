OUT=ecc
SOURCES := $(notdir $(shell find src -name '*.c'))
OBJECTS := $(addprefix build/,$(addsuffix .o,$(basename $(SOURCES))))

.PHONY: default test clean

default: $(OUT) libecc/libecc.a libc/libc.a

test: default
	cd test && $(MAKE)

clean:
	cd test && $(MAKE) clean
	cd libc && $(MAKE) clean
	cd libecc && $(MAKE) clean
	rm -f $(OUT) $(OBJECTS)
	rm -rf build

$(OUT): $(OBJECTS)
	gcc -g -o $(OUT) $^

build/%.o: src/%.c build
	gcc -c -g -Wall -Werror=vla --std=c99 -o $@ $<

libc/libc.a:
	cd libc && $(MAKE)

libecc/libecc.a:
	cd libecc && $(MAKE)

build:
	mkdir -p build
