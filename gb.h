#ifndef GB_H
#define GB_H

#include "cpu.h"
#include "mmu.h"

typedef struct gb
{
    gb_cpu_t *cpu;
    gb_mmu_t *mmu;
} gb_t;

/**
 * Called by the CPU to sync the state of peripherals.
 * 
 * \param gb Gameboy instance
 * \param elapsed_cycles Clock cycles delta for peripherals
 */
gbstatus_e sync_with_cpu(gb_t *gb, int elapsed_cycles);

#endif