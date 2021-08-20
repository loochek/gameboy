Yet another Gameboy emulator.

Implemented (at least I think so) features:
* Emulation of all CPU instructions
* Memory bus with RAM and redirection of MMIO requests to the peripherals
* MBC1, external cartridge RAM with save/load (battery emulation)
* Interrupts
* Timer
* Input
* Quite inaccurate, but full PPU implementation: background, window, sprites, OAM DMA
* Architecture - emulation core with abstract interface and frontends: SFML and libretro

The emulator passes Blargg's cpu_instr, instr_timing, mem_timing tests.

Tested software:
* Super Mario Land - looks playable
* Super Mario Land 2 - looks playable
* The Legend of Zelda: Link's Awakening - looks playable
* Kirby's Dream Land - looks playable
* Tetris - looks playable
* Dr. Mario - looks playable
* Bomb Jack - looks playable
* Ant Soldiers - looks playable
* V-Rally - looks playable
* Oh! Demo - looks as intended

* Road Rash - screen flickering during gameplay, but playable
* Kirby's Dream Land 2 - hangs
* Is That A Demo In Your Pocket? - some visual glitches

## Building

Supports both Clang and GCC.

Just `make` for SFML frontend. Requires CSFML (`sudo apt install libcsfml-dev` in Ubuntu and derivatives).

For libretro frontend, run `make -f Makefile.libretro` (and move `gb_libretro.so` and `gb_libretro.info` to `retroarch/cores`).

##  Gallery

![](screenshots/super_mario_land.png) ![](screenshots/links_awakening.png) ![](screenshots/kirby.png) ![](screenshots/vrally.png) ![](screenshots/ant_soldiers.png) ![](screenshots/oh.png) ![](screenshots/libretro.png)
