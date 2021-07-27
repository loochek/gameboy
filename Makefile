C=clang
CFLAGS=-fsanitize=address -g -lcsfml-graphics -lcsfml-window -lcsfml-system

build:
	$(C) $(CFLAGS) -o gb cpu.c mmu.c ppu.c interrupts.c cart.c mbc_none.c mbc1.c timer.c gb.c gbstatus.c main.c

clean:
	rm -f gb

run: build
	./gb