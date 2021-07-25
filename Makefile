C=clang
CFLAGS=-O2 -fsanitize=address

build:
	$(C) $(CFLAGS) -o gb cpu.c mmu.c interrupts.c cart.c mbc_none.c mbc1.c timer.c gb.c gbstatus.c main.c

clean:
	rm -f gb

run: build
	./gb