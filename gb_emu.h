#ifndef GB_EMU_H
#define GB_EMU_H

#include "gb.h"
#include "cart.h"

/**
 * Gameboy emulator interface
 */
typedef struct
{
    gb_t        gb;
    
    gb_cart_t   cart;
    bool        cart_inserted;

    bool        new_frame_ready;
    const char *framebuffer;

    const char *game_title;
} gb_emu_t;

/**
 * Initializes the instance of the emulator
 * 
 * \param gb_emu Emulator instance
 */
gbstatus_e gb_emu_init(gb_emu_t *gb_emu);

/**
 * Changes current ROM
 * 
 * \param gb_emu Emulator instance
 * \param rom_file_path ROM file path
 */
gbstatus_e gb_emu_change_rom(gb_emu_t *gb_emu, const char *rom_file_path);

/**
 * Resets Gameboy
 * 
 * \param gb_emu Emulator instance
 */
gbstatus_e gb_emu_reset(gb_emu_t *gb_emu);

/**
 * Takes one step of emulation
 * 
 * \param gb_emu Emulator instance
 */
gbstatus_e gb_emu_step(gb_emu_t *gb_emu);

/**
 * Updates state of the joypad
 * 
 * \param gb_emu Emulator instance
 * \param new_state Information about pressed buttons
 */
gbstatus_e gb_emu_update_input(gb_emu_t *gb_emu, int new_state);

/**
 * Deinitializes the instance of the emulator
 * 
 * \param gb_emu Emulator instance
 */
gbstatus_e gb_emu_deinit(gb_emu_t *gb_emu);

#endif