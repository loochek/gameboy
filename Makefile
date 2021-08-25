CC = clang
CFLAGS = -O3 -Isrc/core -Isrc/frontends/sfml
LDFLAGS = -lcsfml-graphics -lcsfml-window -lcsfml-system

SOURCES = \
	src/core/cpu.c \
	src/core/mmu.c \
	src/core/ppu.c \
	src/core/interrupts.c \
	src/core/cart.c \
	src/core/mbc_none.c \
	src/core/mbc1.c \
	src/core/timer.c \
	src/core/joypad.c \
	src/core/gb_emu.c \
	src/core/gbstatus.c \
	src/core/log.c \
	src/frontends/sfml/sfml_main.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = gb

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS)
