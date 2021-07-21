#include "interrupts.h"

const int INT_COUNT = 5;

static interrupt_e int_order[] = 
{
    INT_VBLANK,
    INT_LCDC,
    INT_TIMA,
    INT_SERIAL,
    INT_JOYPAD
};

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

gbstatus_e gb_int_request(gb_int_controller_t *ctrl, interrupt_e intr)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an interrupt controller instance");
        return status;
    }

    ctrl->reg_if |= intr;
    return GBSTATUS_OK;
}

gbstatus_e int_step(gb_int_controller_t *ctrl)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ctrl->reg_ie & ctrl->reg_if & 0x1F)
    {
        // Pending interrupts are present
        
        for (int int_num = 0; int_num < INT_COUNT; int_num++)
        {
            interrupt_e intr = int_order[int_num];

            if (ctrl->reg_ie & ctrl->reg_if & intr)
            {
                status = cpu_irq(ctrl->gb->cpu, isr_addr[int_num]);
                if (status == GBSTATUS_INT_DISABLED)
                    break;

                // check for unexpected situations
                GBCHK(status);

                ctrl->reg_if &= ~intr;
                break;
            }
        }
    }

    return GBSTATUS_OK;
}