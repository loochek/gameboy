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
void joypad_init(gb_joypad_t *joypad, struct gb *gb);

/**
 * Resets the joypad
 * 
 * \param joypad Joypad instance
 * \param gb Parent GB instance
 */
void joypad_reset(gb_joypad_t *joypad);

/**
 * Emulates JOYP register reading
 * 
 * \param joypad Joypad instance
 * \return Register value
 */
uint8_t joypad_joyp_read(gb_joypad_t *joypad);

/**
 * Emulates writing to the JOYP register
 * 
 * \param joypad Joypad instance
 * \param value Value to write
 */
void joypad_joyp_write(gb_joypad_t *joypad, uint8_t value);

/**
 * Updates state of the joypad
 * 
 * \param joypad Joypad instance
 * \param new_state Information about pressed buttons
 */
void joypad_update(gb_joypad_t *joypad, int new_state);

#endif