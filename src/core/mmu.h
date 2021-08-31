#ifndef MMU_H
#define MMU_H

#include <stdbool.h>
#include <stdint.h>
#include "gbstatus.h"

struct gb;
struct gb_cart;

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

    /// Current cartridge
    struct gb_cart *cart;

    /// Pointer to the parent Gameboy structure
    struct gb *gb;

    /// First 256 bytes are mapped to the bootrom instead of cartridge after power-on
    bool bootrom_mapped;
} gb_mmu_t;

/**
 * Initializes the instance of the MMU
 * 
 * \param mmu MMU instance
 * \param gb Parent GB instance
 * \param rom_path ROM to load
 */
gbstatus_e mmu_init(gb_mmu_t *mmu, struct gb *gb);

/**
 * Resets the MMU
 * 
 * \param mmu MMU instance
 */
void mmu_reset(gb_mmu_t *mmu);

/**
 * Disables BootROM mapping
 * 
 * \param mmu MMU instance
 */
void mmu_skip_bootrom(gb_mmu_t *mmu);

/**
 * Changes the cartridge
 * 
 * \param mmu MMU instance
 * \param cart Cartridge instance
 */
void mmu_switch_cart(gb_mmu_t *mmu, struct gb_cart *cart);

/**
 * Emulates a memory read request from the CPU
 * 
 * \param mmu MMU instance
 * \param addr Address to read
 * \return Byte read
 */
uint8_t mmu_read(gb_mmu_t *mmu, uint16_t addr);

/**
 * Emulates a memory write request from the CPU
 * 
 * \param mmu MMU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void mmu_write(gb_mmu_t *mmu, uint16_t addr, uint8_t byte);

/**
 * Deinitializes the instance of the MMU
 * 
 * \param mmu MMU instance
 */
void mmu_deinit(gb_mmu_t *mmu);

#endif