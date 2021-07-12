#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "gbstatus.h"

/**
 * Represents Gameboy memory bus. 
 * Handles RAM, cartridge, bank switching and passes MMIO requests to peripheral devices. 
 */
typedef struct gb_mmu
{

} gb_mmu_t;

/**
 * Emulates a memory read request from the CPU
 * 
 * \param mmu MMU instance
 * \param addr Address to read
 * \param byte_out Where to store read byte
 */
gbstatus_e mmu_read(gb_mmu_t *mmu, uint16_t addr, uint8_t *byte_out);

/**
 * Emulates a memory write request from the CPU
 * 
 * \param mmu MMU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
gbstatus_e mmu_write(gb_mmu_t *mmu, uint16_t addr, uint8_t byte);

#endif