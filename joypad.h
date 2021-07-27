#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdint.h>
#include "gbstatus.h"

struct gb;

typedef enum
{
    BUTTON_RIGHT  = 1,
    BUTTON_LEFT   = 2,
    BUTTON_UP     = 4,
    BUTTON_DOWN   = 8,
    BUTTON_A      = 16,
    BUTTON_B      = 32,
    BUTTON_SELECT = 64,
    BUTTON_START  = 128
} gb_buttons_e;

typedef struct
{
    int state;

    uint8_t reg_joyp;

    /// Pointer to the parent Gameboy structure
    struct gb *gb;
} gb_joypad_t;

/**
 * Initializes the instance of the joypad
 * 
 * \param joypad Joypad instance
 * \param gb Parent GB instance
 */
gbstatus_e joypad_init(gb_joypad_t *joypad, struct gb *gb);

/**
 * Resets the joypad
 * 
 * \param joypad Joypad instance
 * \param gb Parent GB instance
 */
gbstatus_e joypad_reset(gb_joypad_t *joypad);

/**
 * Emulates JOYP register reading
 * 
 * \param joypad Joypad instance
 * \param value_ptr Where to write value
 */
gbstatus_e joypad_joyp_read(gb_joypad_t *joypad, uint8_t *value_out);

/**
 * Emulates writing to the JOYP register
 * 
 * \param joypad Joypad instance
 * \param value Value to write
 */
gbstatus_e joypad_joyp_write(gb_joypad_t *joypad, uint8_t value);

/**
 * Updates state of the joypad
 * 
 * \param joypad Joypad instance
 * \param new_state Information about pressed buttons
 */
gbstatus_e joypad_update(gb_joypad_t *joypad, int new_state);

#endif