#include <assert.h>
#include "joypad.h"
#include "gb.h"

/// Constructs correct JOYP value from P14, P15 and joypad state
static void joypad_update_reg(gb_joypad_t *joypad);

void joypad_init(gb_joypad_t *joypad, struct gb *gb)
{
    assert(joypad != NULL);
    assert(gb != NULL);

    joypad->gb = gb;

    joypad_reset(joypad);
}

void joypad_reset(gb_joypad_t *joypad)
{
    assert(joypad != NULL);

    // Nothing is pressed
    joypad->state    = 0;
    joypad->reg_joyp = 0xCF;
}

uint8_t joypad_joyp_read(gb_joypad_t *joypad)
{
    assert(joypad != NULL);

    return joypad->reg_joyp;
}

void joypad_joyp_write(gb_joypad_t *joypad, uint8_t value)
{
    assert(joypad != NULL);

    // Only P14 and P15 are needed actually
    joypad->reg_joyp = value;
    joypad_update_reg(joypad);
}

void joypad_update(gb_joypad_t *joypad, int new_state)
{
    assert(joypad != NULL);

    joypad->state = new_state;
    joypad_update_reg(joypad);

    if ((joypad->reg_joyp & 0xF) != 0xF)
        int_request(&joypad->gb->intr_ctrl, INT_JOYPAD);
}

static void joypad_update_reg(gb_joypad_t *joypad)
{
    // Reset P10-P13
    joypad->reg_joyp &= 0xF0;

    int selected_group = (joypad->reg_joyp >> 4) & 0x3;
    if (selected_group == 2)
        joypad->reg_joyp |= (~joypad->state) & 0xF;
    else if (selected_group == 1)
        joypad->reg_joyp |= ((~joypad->state) >> 4) & 0xF;
    else
        joypad->reg_joyp |= 0xF;
}