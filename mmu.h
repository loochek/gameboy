#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "gbstatus.h"

struct gb;

/**
 * Represents Gameboy memory bus. 
 * Handles RAM, cartridge and passes MMIO requests to peripheral devices. 
 */
typedef struct gb_mmu
{
    /// Internal RAM
    uint8_t *ram;

    /// Internal RAM 2
    uint8_t *hram;

    /// Cartridge ROM (no MBC for now)
    uint8_t *rom;

    /// Pointer to the parent Gameboy structure
    struct gb *gb;

    /// Debug thing
    char serial_data;
} gb_mmu_t;

/**
 * Initializes the instance of the MMU
 * 
 * \param mmu MMU instance
 * \param gb Parent GB instance
 * \param rom_path ROM to load
 */
gbstatus_e mmu_init(gb_mmu_t *mmu, struct gb *gb, const char *rom_path);

/**
 * Resets the MMU
 * 
 * \param mmu MMU instance
 */
gbstatus_e mmu_reset(gb_mmu_t *mmu);

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

/**
 * Deinitializes the instance of the MMU
 * 
 * \param mmu MMU instance
 */
gbstatus_e mmu_deinit(gb_mmu_t *mmu);

#endif