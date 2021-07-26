#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include "gbstatus.h"

struct gb;

typedef enum
{
    STATE_OBJ_SEARCH,
    STATE_DRAWING,
    STATE_HBLANK,
    STATE_HBLANK_INC,
    STATE_VBLANK,
    STATE_VBLANK_INC,
    STATE_VBLANK_LAST_LINE
} ppu_state_e;

/**
 * Gameboy PPU representation
 */
typedef struct gb_ppu
{
    uint8_t reg_lcdc;
    uint8_t reg_stat;
    uint8_t reg_ly;
    uint8_t reg_lyc;

    char *framebuffer;

    uint8_t *vram;
    uint8_t *oam;

    /// Clock cycles counter for PPU updating
    int cycles_counter;

    int clocks_to_next_state;

    /// LCDC interrupt can be requested once per line
    bool lcdc_blocked;

    ppu_state_e curr_state;

    /// Pointer to the parent Gameboy structure
    struct gb *gb;
} gb_ppu_t;

/**
 * Initializes the instance of the PPU
 * 
 * \param ppu PPU instance
 * \param gb Parent GB instance
 */
gbstatus_e ppu_init(gb_ppu_t *ppu, struct gb *gb);

/**
 * Resets the PPU
 * 
 * \param ppu PPU instance
 */
gbstatus_e ppu_reset(gb_ppu_t *ppu);

/**
 * Updates state of the PPU according to elapsed cycles
 * 
 * \param ppu PPU instance
 * \param elapsed_cycles Clock cycles delta
 */
gbstatus_e ppu_update(gb_ppu_t *ppu, int elapsed_cycles);

/**
 * Emulates LCDC register reading
 * 
 * \param ppu PPU instance
 * \param value_ptr Where to write value
 */
gbstatus_e ppu_lcdc_read(gb_ppu_t *ppu, uint8_t *value_out);

/**
 * Emulates STAT register reading
 * 
 * \param ppu PPU instance
 * \param value_ptr Where to write value
 */
gbstatus_e ppu_stat_read(gb_ppu_t *ppu, uint8_t *value_out);

/**
 * Emulates LY register reading
 * 
 * \param ppu PPU instance
 * \param value_ptr Where to write value
 */
gbstatus_e ppu_ly_read(gb_ppu_t *ppu, uint8_t *value_out);

/**
 * Emulates LYC register reading
 * 
 * \param ppu PPU instance
 * \param value_ptr Where to write value
 */
gbstatus_e ppu_lyc_read(gb_ppu_t *ppu, uint8_t *value_out);

/**
 * Emulates a memory read request to the VRAM
 * 
 * \param ppu PPU instance
 * \param addr Address to read
 * \param byte_out Where to store read byte
 */
gbstatus_e ppu_vram_read(gb_ppu_t *ppu, uint16_t addr, uint8_t *byte_out);

/**
 * Emulates a memory read request to the OAM
 * 
 * \param ppu PPU instance
 * \param addr Address to read
 * \param byte_out Where to store read byte
 */
gbstatus_e ppu_oam_read(gb_ppu_t *ppu, uint16_t addr, uint8_t *byte_out);

/**
 * Emulates writing to the LCDC register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
gbstatus_e ppu_lcdc_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the STAT register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
gbstatus_e ppu_stat_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the LY register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
gbstatus_e ppu_ly_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the LYC register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
gbstatus_e ppu_lyc_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates a memory write request to the VRAM
 * 
 * \param ppu PPU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
gbstatus_e ppu_vram_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte);

/**
 * Emulates a memory write request to the OAM
 * 
 * \param ppu PPU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
gbstatus_e ppu_oam_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte);

/**
 * Deinitializes the instance of the PPU
 * 
 * \param ppu PPU instance
 */
gbstatus_e ppu_deinit(gb_ppu_t *ppu);

#endif