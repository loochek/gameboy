C=clang
CFLAGS=

build:
	$(C) $(CFLAGS) -o gb cpu.c mmu.c interrupts.c gb.c gbstatus.c