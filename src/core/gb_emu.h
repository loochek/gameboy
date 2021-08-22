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
} gb_emu_t;

/**
 * Initializes the instance of the emulator
 * 
 * \param gb_emu Emulator instance
 */
gbstatus_e gb_emu_init(gb_emu_t *gb_emu);

/**
 * Returns pointer to the PPU framebuffer
 * 
 * \param gb_emu Emulator instance
 * \param fb_ptr_out Where to write pointer
 */
void gb_emu_framebuffer_ptr(gb_emu_t *gb_emu, const char **fb_ptr_out);

/**
 * Returns pointer to the PPU frame ready flag
 * 
 * \param gb_emu Emulator instance
 * \param ready_out Where to write pointer to the flag
 */
void gb_emu_frame_ready_ptr(gb_emu_t *gb_emu, const bool **ready_ptr_out);

/**
 * Resets PPU frame ready flag
 * 
 * \param gb_emu Emulator instance
 */
void gb_emu_grab_frame(gb_emu_t *gb_emu);

/**
 * Returns pointer to the game title or NULL if no ROM loaded
 * 
 * \param gb_emu Emulator instance
 * \param title_out Where to write pointer
 */
void gb_emu_game_title_ptr(gb_emu_t *gb_emu, const char **title_out);

/**
 * Changes current ROM
 * 
 * \param gb_emu Emulator instance
 * \param rom_file_path ROM file path
 */
gbstatus_e gb_emu_change_rom(gb_emu_t *gb_emu, const char *rom_file_path);

/**
 * Unloads current ROM
 * 
 * \param gb_emu Emulator instance
 */
void gb_emu_unload_rom(gb_emu_t *gb_emu);

/**
 * Resets Gameboy
 * 
 * \param gb_emu Emulator instance
 */
void gb_emu_reset(gb_emu_t *gb_emu);

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
void gb_emu_update_input(gb_emu_t *gb_emu, int new_state);

/**
 * Deinitializes the instance of the emulator
 * 
 * \param gb_emu Emulator instance
 */
void gb_emu_deinit(gb_emu_t *gb_emu);

#endif