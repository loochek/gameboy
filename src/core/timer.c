#include <assert.h>
#include "timer.h"
#include "gb.h"

/// Divider increment period in clock cycles
#define DIV_TICK_PERIOD 256

// Timer increment periods in clock cycles
static int timer_periods[] =
{
    1024, // 00
    16,   // 01
    64,   // 10
    256   // 11
};

void timer_init(gb_timer_t *timer, struct gb *gb)
{
    assert(timer != NULL);
    assert(gb != NULL);

    timer->gb = gb;
    timer_reset(timer);
}

void timer_reset(gb_timer_t *timer)
{
    assert(timer != NULL);

    timer->reg_div  = 0x00;
    timer->reg_tima = 0;
    timer->reg_tma  = 0;
    timer->reg_tac  = 0;

    timer->div_cycles   = 0;
    timer->timer_cycles = 0;
}

void timer_div_write(gb_timer_t *timer, uint8_t value)
{
    assert(timer != NULL);

    timer->reg_div = 0x00;
}

void timer_tima_write(gb_timer_t *timer, uint8_t value)
{
    assert(timer != NULL);
    
    timer->reg_tima = value;
}

void timer_tma_write(gb_timer_t *timer, uint8_t value)
{
    assert(timer != NULL);

    timer->reg_tma = value;
}

void timer_tac_write(gb_timer_t *timer, uint8_t value)
{
    assert(timer != NULL);

    timer->reg_tac = value;
}

uint8_t timer_div_read(gb_timer_t *timer)
{
    assert(timer != NULL);

    return timer->reg_div;
}

uint8_t timer_tima_read(gb_timer_t *timer)
{
    assert(timer != NULL);

    return timer->reg_tima;
}

uint8_t timer_tma_read(gb_timer_t *timer)
{
    assert(timer != NULL);

    return timer->reg_tma;
}

uint8_t timer_tac_read(gb_timer_t *timer)
{
    assert(timer != NULL);
    
    return timer->reg_tac;
}

void timer_update(gb_timer_t *timer, int elapsed_cycles)
{
    assert(timer != NULL);

    timer->div_cycles += elapsed_cycles;
    int div_ticks = timer->div_cycles / DIV_TICK_PERIOD;
    timer->div_cycles %= DIV_TICK_PERIOD;

    timer->reg_div += div_ticks;

    if (timer->reg_tac & 0x04)
    {
        // Timer is enabled

        int timer_period = timer_periods[timer->reg_tac & 0x3];

        timer->timer_cycles += elapsed_cycles;
        int timer_ticks = timer->timer_cycles / timer_period;
        timer->timer_cycles %= timer_period;

        if (timer->reg_tima + timer_ticks > 255)
        {
            timer->reg_tima = timer->reg_tma;
            int_request(&timer->gb->intr_ctrl, INT_TIMA);
        }
        else
            timer->reg_tima += timer_ticks;
    }
}