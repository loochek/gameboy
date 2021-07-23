#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdlib.h>
#include <stdint.h>
#include "gbstatus.h"

// Interrupts are ordered by bit position/priority

typedef enum
{
    INT_VBLANK,
    INT_LCDC,
    INT_TIMA,
    INT_SERIAL,
    INT_JOYPAD
} interrupt_e;

struct gb;

/**
 * Representation of Gameboy interrupt controller
 */
typedef struct
{
    // Interrupt enable register
    uint8_t reg_ie;

    // Interrupt flags register
    uint8_t reg_if;

    /// Pointer to the parent Gameboy structure
    struct gb *gb;
} gb_int_controller_t;

/**
 * Initializes the instance of the interrupt controller
 * 
 * \param ctrl Interrupt controller instance
 * \param gb Parent GB instance
 */
gbstatus_e int_init(gb_int_controller_t *ctrl, struct gb *gb);

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
 * Makes one step of the interrupt controller logic
 * 
 * \param ctrl Interrupt controller instance
 */
gbstatus_e int_step(gb_int_controller_t *ctrl);

#endif