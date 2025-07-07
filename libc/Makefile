OUT=libc.a
SOURCES := $(notdir $(shell find src -name '*.c') $(shell find src -name '*.s'))
OBJECTS := $(addprefix build/,$(addsuffix .o,$(basename $(SOURCES))))

.PHONY: default clean

default: $(OUT)

clean:
	rm -f $(OUT) $(OBJECTS)
	rm -rf build

$(OUT): $(OBJECTS)
	ar r $(OUT) $^

build/%.o: src/%.s build
	x86_64-linux-gnu-as -o $@ $<

build/%.o: src/%.c build
	./../ecc -c -o $@ $<

build:
	mkdir -p build
