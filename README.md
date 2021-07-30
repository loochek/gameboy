(Maybe in the future) Gameboy emulator.

Implemented (at least I think so) features:
* Emulation of all CPU instructions
* Memory bus with RAM and redirection of MMIO requests to the peripherals
* MBC1 and external cartridge RAM
* Interrupts
* Timer
* Input
* (Inaccurate and quite faulty) PPU implementation: background, window, sprites, OAM DMA

The emulator passes Blargg's cpu_instr, instr_timing, mem_timing tests.

Tested games:
* Super Mario Land - looks playable
* The Legend of Zelda: Link's Awakening - looks playable
* Tetris - menu works, but there is garbage on the screen instead of main game screen

## Building

Just `make`. Supports both Clang and GCC. Requires CSFML (`sudo apt install libcsfml-dev` in Ubuntu and derivatives)

##  Gallery

![](screenshots/super_mario_land.png) ![](screenshots/links_awakening.png)
