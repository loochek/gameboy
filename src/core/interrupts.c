#include <assert.h>
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

void int_init(gb_int_controller_t *ctrl, gb_t *gb)
{
    assert(ctrl != NULL);
    assert(gb   != NULL);

    ctrl->gb = gb;
    int_reset(ctrl);
}

void int_reset(gb_int_controller_t *ctrl)
{
    assert(ctrl != NULL);

    ctrl->reg_ie = 0x00;
    ctrl->reg_if = 0x00;
}

void int_if_write(gb_int_controller_t *ctrl, uint8_t value)
{
    assert(ctrl != NULL);

    ctrl->reg_if = value;
}

void int_ie_write(gb_int_controller_t *ctrl, uint8_t value)
{
    assert(ctrl != NULL);

    ctrl->reg_ie = value;
}

uint8_t int_if_read(gb_int_controller_t *ctrl)
{
    assert(ctrl != NULL);

    return ctrl->reg_if;
}

uint8_t int_ie_read(gb_int_controller_t *ctrl)
{
    assert(ctrl != NULL);

    return ctrl->reg_ie;
}

void int_request(gb_int_controller_t *ctrl, interrupt_e intr)
{
    assert(ctrl != NULL);

    ctrl->reg_if |= 1 << intr;
}

void int_step(gb_int_controller_t *ctrl)
{
    assert(ctrl != NULL);

    for (interrupt_e intr = 0; intr < INT_COUNT; intr++)
    {
        int intr_mask = 1 << intr;

        if (ctrl->reg_ie & ctrl->reg_if & intr_mask)
        {
            if (!cpu_irq(&ctrl->gb->cpu, isr_addr[intr]))
                break;

            ctrl->reg_if &= ~intr_mask;
            break;
        }
    }
}