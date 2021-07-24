#ifndef GB_H
#define GB_H

#include "cpu.h"
#include "mmu.h"
#include "interrupts.h"
#include "timer.h"

typedef struct gb
{
    gb_cpu_t            *cpu;
    gb_mmu_t            *mmu;
    gb_int_controller_t *intr_ctrl;
    gb_timer_t          *timer;
} gb_t;

/**
 * Updates the state of peripherals according to elapsed clock cycles. 
 * Called by the CPU
 * 
 * \param gb Gameboy instance
 * \param elapsed_cycles Clock cycles delta
 */
gbstatus_e sync_with_cpu(gb_t *gb, int elapsed_cycles);

#endif