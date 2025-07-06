OUT=libecc.a
OBJS=crt0.o
BUILD=$(addprefix build/, $(OBJS))

.PHONY: default buildfolder clean

default: buildfolder $(OUT)

buildfolder:
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