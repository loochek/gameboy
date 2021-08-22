#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "gbstatus.h"

struct gb;

/**
 * Represents timer and divider. 
 */
typedef struct gb_timer
{
    uint8_t reg_div;
    uint8_t reg_tima;
    uint8_t reg_tma;
    uint8_t reg_tac;

    /// Clock cycles counter for updating divider
    int div_cycles;

    /// Clock cycles counter for updating timer
    int timer_cycles;

    /// Pointer to the parent Gameboy structure
    struct gb *gb;
} gb_timer_t;

/**
 * Initializes the instance of the timer
 * 
 * \param timer Timer instance
 * \param gb Parent GB instance
 */
void timer_init(gb_timer_t *timer, struct gb *gb);

/**
 * Resets the timer
 * 
 * \param timer Timer instance
 */
void timer_reset(gb_timer_t *timer);

/**
 * Emulates writing to the DIV register 
 * 
 * \param timer Timer instance
 * \param value Value to write
 */
void timer_div_write(gb_timer_t *timer, uint8_t value);

/**
 * Emulates writing to the TIMA register 
 * 
 * \param timer Timer instance
 * \param value Value to write
 */
void timer_tima_write(gb_timer_t *timer, uint8_t value);

/**
 * Emulates writing to the TMA register 
 * 
 * \param timer Timer instance
 * \param value Value to write
 */
void timer_tma_write(gb_timer_t *timer, uint8_t value);

/**
 * Emulates writing to the TAC register 
 * 
 * \param timer Timer instance
 * \param value Value to write
 */
void timer_tac_write(gb_timer_t *timer, uint8_t value);

/**
 * Emulates DIV register reading
 * 
 * \param timer Timer instance
 * \return Register value
 */
uint8_t timer_div_read(gb_timer_t *timer);

/**
 * Emulates TIMA register reading
 * 
 * \param timer Timer instance
 * \return Register value
 */
uint8_t timer_tima_read(gb_timer_t *timer);

/**
 * Emulates TMA register reading
 * 
 * \param timer Timer instance
 * \return Register value
 */
uint8_t timer_tma_read(gb_timer_t *timer);

/**
 * Emulates TAC register reading
 * 
 * \param timer Timer instance
 * \return Register value
 */
uint8_t timer_tac_read(gb_timer_t *timer);

/**
 * Updates state of the timer according to elapsed cycles
 * 
 * \param timer Timer instance
 * \param elapsed_cycles Clock cycles delta
 */
void timer_update(gb_timer_t *timer, int elapsed_cycles);

#endif