(Maybe in the future) Gameboy emulator.

Implemented (at least I think so) features:
* Emulation of all CPU instructions
* Memory bus with RAM and redirection of MMIO requests to the peripherals
* MBC1 and external cartridge RAM
* Interrupts
* Timer

The emulator passes Blargg's cpu_instr, instr_timing, mem_timing tests.
