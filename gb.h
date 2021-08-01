#ifndef GB_H
#define GB_H

#include "cpu.h"
#include "mmu.h"
#include "interrupts.h"
#include "timer.h"
#include "ppu.h"
#include "joypad.h"

/**
 * Abstract model of the Gameboy
 */
typedef struct gb
{
    gb_cpu_t            cpu;
    gb_mmu_t            mmu;
    gb_ppu_t            ppu;
    gb_int_controller_t intr_ctrl;
    gb_timer_t          timer;
    gb_joypad_t         joypad;
} gb_t;

#endif