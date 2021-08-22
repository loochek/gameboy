#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include "gbstatus.h"

#define GB_SCREEN_WIDTH 160
#define GB_SCREEN_HEIGHT 144

#define MAX_SPRITE_PER_LINE 10

struct gb;

typedef enum
{
    STATE_OBJ_SEARCH,
    STATE_DRAWING,
    STATE_HBLANK,
    STATE_HBLANK_INC,
    STATE_VBLANK,
    STATE_VBLANK_INC,
    STATE_VBLANK_LAST_LINE,
    STATE_VBLANK_LAST_LINE_INC
} ppu_state_e;

/**
 * Gameboy PPU representation
 */
typedef struct gb_ppu
{
    // Control registers

    uint8_t reg_lcdc;
    uint8_t reg_stat;
    uint8_t reg_ly;
    uint8_t reg_lyc;

    // Background scrolling

    uint8_t reg_scx;
    uint8_t reg_scy;

    // Window position

    uint8_t reg_wx;
    uint8_t reg_wy;

    // Palletes

    uint8_t reg_bgp;
    uint8_t reg_obp0;
    uint8_t reg_obp1;

    uint8_t *vram;
    uint8_t *oam;

    /// Internal window line counter
    int window_line;

    /// WY update takes effect only on next frame
    int delayed_wy;

    /// Clock cycles counter for PPU updating
    int cycles_counter;

    int clocks_to_next_state;

    /// LCDC interrupt can be requested once per line
    bool lcdc_blocked;

    ppu_state_e next_state;

    bool new_frame_ready;
    char *framebuffer;

    /// Holds original BG and Window colors to handle OBJ priority bit
    char *bg_scanline_buffer;

    int sprite_draw_order[MAX_SPRITE_PER_LINE];

    /// Draw order size
    int line_sprite_count;

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
void ppu_reset(gb_ppu_t *ppu);

/**
 * Updates state of the PPU according to elapsed cycles
 * 
 * \param ppu PPU instance
 * \param elapsed_cycles Clock cycles delta
 */
void ppu_update(gb_ppu_t *ppu, int elapsed_cycles);

/**
 * Emulates LCDC register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_lcdc_read(gb_ppu_t *ppu);

/**
 * Emulates STAT register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_stat_read(gb_ppu_t *ppu);

/**
 * Emulates LY register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_ly_read(gb_ppu_t *ppu);

/**
 * Emulates LYC register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_lyc_read(gb_ppu_t *ppu);

/**
 * Emulates SCX register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_scx_read(gb_ppu_t *ppu);

/**
 * Emulates SCY register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_scy_read(gb_ppu_t *ppu);

/**
 * Emulates WX register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_wx_read(gb_ppu_t *ppu);

/**
 * Emulates WY register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_wy_read(gb_ppu_t *ppu);

/**
 * Emulates BGP register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_bgp_read(gb_ppu_t *ppu);

/**
 * Emulates OBP0 register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_obp0_read(gb_ppu_t *ppu);

/**
 * Emulates OBP1 register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_obp1_read(gb_ppu_t *ppu);

/**
 * Emulates DMA register reading
 * 
 * \param ppu PPU instance
 * \return Register value
 */
uint8_t ppu_dma_read(gb_ppu_t *ppu);

/**
 * Emulates a memory read request to the VRAM
 * 
 * \param ppu PPU instance
 * \param addr Address to read
 * \return Byte read
 */
uint8_t ppu_vram_read(gb_ppu_t *ppu, uint16_t addr);

/**
 * Emulates a memory read request to the OAM
 * 
 * \param ppu PPU instance
 * \param addr Address to read
 * \return Byte read
 */
uint8_t ppu_oam_read(gb_ppu_t *ppu, uint16_t addr);

/**
 * Emulates writing to the LCDC register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_lcdc_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the STAT register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_stat_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the LY register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_ly_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the LYC register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_lyc_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the SCX register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_scx_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the SCY register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_scy_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the WX register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_wx_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the WY register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_wy_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the BGP register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_bgp_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the OBP0 register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_obp0_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the OBP1 register
 * 
 * \param ppu PPU instance
 * \param value Value to write
 */
void ppu_obp1_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates writing to the DMA register
 * 
 * \param ppu PPU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void ppu_dma_write(gb_ppu_t *ppu, uint8_t value);

/**
 * Emulates a memory write request to the VRAM
 * 
 * \param ppu PPU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void ppu_vram_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte);

/**
 * Emulates a memory write request to the OAM
 * 
 * \param ppu PPU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void ppu_oam_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte);

/**
 * Deinitializes the instance of the PPU
 * 
 * \param ppu PPU instance
 */
void ppu_deinit(gb_ppu_t *ppu);

#endif