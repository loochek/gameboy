C=clang
CFLAGS=-O2

build:
	$(C) $(CFLAGS) -o gb cpu.c mmu.c interrupts.c gb.c gbstatus.c main.c

clean:
	rm -f gb