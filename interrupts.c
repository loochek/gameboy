#include "interrupts.h"
#include "gb.h"

#define INT_COUNT 5

static uint8_t isr_addr[] = 
{
    0x0040, // VBLANK
    0x0048, // LCDC
    0x0050, // TIMA
    0x0058, // SERIAL
    0x0060  // JOYPAD
};

gbstatus_e int_init(gb_int_controller_t *ctrl, gb_t *gb)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as interrupt controller instance");
        return status;
    }

    ctrl->gb = gb;
    GBCHK(int_reset(ctrl));
    
    return GBSTATUS_OK;
}

gbstatus_e int_reset(gb_int_controller_t *ctrl)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as interrupt controller instance");
        return status;
    }

    ctrl->reg_ie = 0x00;
    ctrl->reg_if = 0x00;

    return GBSTATUS_OK;
}

gbstatus_e int_if_write(gb_int_controller_t *ctrl, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an interrupt controller instance");
        return status;
    }

    ctrl->reg_if = value;
    return GBSTATUS_OK;
}

gbstatus_e int_ie_write(gb_int_controller_t *ctrl, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an interrupt controller instance");
        return status;
    }

    ctrl->reg_ie = value;
    return GBSTATUS_OK;
}

gbstatus_e int_if_read(gb_int_controller_t *ctrl, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an interrupt controller instance");
        return status;
    }

    *value_out = ctrl->reg_if;
    return GBSTATUS_OK;
}

gbstatus_e int_ie_read(gb_int_controller_t *ctrl, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an interrupt controller instance");
        return status;
    }

    *value_out = ctrl->reg_ie;
    return GBSTATUS_OK;
}

gbstatus_e int_request(gb_int_controller_t *ctrl, interrupt_e intr)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an interrupt controller instance");
        return status;
    }

    ctrl->reg_if |= 1 << intr;
    return GBSTATUS_OK;
}

gbstatus_e int_step(gb_int_controller_t *ctrl)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an interrupt controller instance");
        return status;
    }

    for (interrupt_e intr = 0; intr < INT_COUNT; intr++)
    {
        int intr_mask = 1 << intr;

        if (ctrl->reg_ie & ctrl->reg_if & intr_mask)
        {
            status = cpu_irq(ctrl->gb->cpu, isr_addr[intr]);
            if (status == GBSTATUS_INT_DISABLED)
                break;

            // check for unexpected situations
            GBCHK(status);

            ctrl->reg_if &= ~intr_mask;
            break;
        }
    }

    return GBSTATUS_OK;
}