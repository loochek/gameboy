#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdlib.h>
#include <stdint.h>
#include "gbstatus.h"
#include "gb.h"

typedef enum
{
    INT_VBLANK = 1,
    INT_LCDC   = 2,
    INT_TIMA   = 4,
    INT_SERIAL = 8,
    INT_JOYPAD = 16
} interrupt_e;

typedef struct
{
    // Interrupt enable register
    uint8_t reg_ie;

    // Interrupt flags register
    uint8_t reg_if;

    /// Pointer to the Gameboy structure
    gb_t *gb;
} gb_int_controller_t;

/**
 * Initializes the instance of the interrupt controller
 * 
 * \param ctrl Interrupt controller instance
 * \param gb Parent GB instance
 */
gbstatus_e int_init(gb_int_controller_t *ctrl, gb_t *gb);

/**
 * Resets the interrupt controller
 * 
 * \param ctrl Interrupt controller instance
 */
gbstatus_e int_reset(gb_int_controller_t *ctrl);

/**
 * Makes an interrupt request
 * 
 * \param ctrl Interrupt controller instance
 * \param intr Interrupt to request
 */
gbstatus_e int_request(gb_int_controller_t *ctrl, interrupt_e intr);

/**
 * Makes one step of an interrupt controller logic
 * 
 * \param ctrl Interrupt controller instance
 */
gbstatus_e int_step(gb_int_controller_t *ctrl);

#endif