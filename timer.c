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

gbstatus_e timer_init(gb_timer_t *timer, struct gb *gb)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    timer->gb = gb;

    GBCHK(timer_reset(timer));    
    return GBSTATUS_OK;
}

gbstatus_e timer_reset(gb_timer_t *timer)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    timer->reg_div  = 0x00;
    timer->reg_tima = 0;
    timer->reg_tma  = 0;
    timer->reg_tac  = 0;

    timer->div_cycles   = 0;
    timer->timer_cycles = 0;

    return GBSTATUS_OK;
}

gbstatus_e timer_div_write(gb_timer_t *timer, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    timer->reg_div = 0x00;
    return GBSTATUS_OK;
}

gbstatus_e timer_tima_write(gb_timer_t *timer, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    timer->reg_tima = value;
    return GBSTATUS_OK;
}

gbstatus_e timer_tma_write(gb_timer_t *timer, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    timer->reg_tma = value;
    return GBSTATUS_OK;
}

gbstatus_e timer_tac_write(gb_timer_t *timer, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    timer->reg_tac = value;
    return GBSTATUS_OK;
}

gbstatus_e timer_div_read(gb_timer_t *timer, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    *value_out = timer->reg_div;
    return GBSTATUS_OK;
}

gbstatus_e timer_tima_read(gb_timer_t *timer, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    *value_out = timer->reg_tima;
    return GBSTATUS_OK;
}

gbstatus_e timer_tma_read(gb_timer_t *timer, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    *value_out = timer->reg_tma;
    return GBSTATUS_OK;
}

gbstatus_e timer_tac_read(gb_timer_t *timer, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

    *value_out = timer->reg_tac;
    return GBSTATUS_OK;
}

gbstatus_e timer_update(gb_timer_t *timer, int elapsed_cycles)
{
    gbstatus_e status = GBSTATUS_OK;

    if (timer == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as timer instance");
        return status;
    }

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
            GBCHK(int_request(timer->gb->intr_ctrl, INT_TIMA));
        }
        else
            timer->reg_tima += timer_ticks;
    }

    return GBSTATUS_OK;
}