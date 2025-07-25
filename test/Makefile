SOURCES := $(shell find . -name '*.c')

.PHONY: test clean

test: actual asm diff ../libc/libc.a ../libecc/libecc.a
	./test.sh $(SOURCES)

clean:
	rm -rf actual
	rm -rf asm
	rm -rf diff

actual:
	mkdir -p actual

asm:
	mkdir -p asm

diff:
	mkdir -p diff

../libc/libc.a:
	cd ../libc && $(MAKE)

../libecc/libecc.a:
	cd ../libecc && $(MAKE)
