OUT=libecc.a
OBJS=crt0.o
BUILD=$(addprefix build/, $(OBJS))

default: buildfolder $(OUT)

buildfolder:
	mkdir -p build

$(OUT): $(BUILD)
		ar r $(OUT) $^

build/%.o: src/%.s
		as -o $@ $<

clean:
		rm -f $(OUT) $(BUILD)
		rm -rf build