#include "joypad.h"
#include "gb.h"

/// Constructs correct JOYP value from P14, P15 and joypad state
static void joypad_update_reg(gb_joypad_t *joypad);

gbstatus_e joypad_init(gb_joypad_t *joypad, struct gb *gb)
{
    gbstatus_e status = GBSTATUS_OK;

    if (joypad == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an joypad instance");
        return status;
    }

    joypad->gb = gb;

    GBCHK(joypad_reset(joypad));
    return GBSTATUS_OK;
}

gbstatus_e joypad_reset(gb_joypad_t *joypad)
{
    gbstatus_e status = GBSTATUS_OK;

    if (joypad == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an joypad instance");
        return status;
    }

    // Nothing is pressed
    joypad->state    = 0;
    joypad->reg_joyp = 0xCF;
    return GBSTATUS_OK;
}

gbstatus_e joypad_joyp_read(gb_joypad_t *joypad, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (joypad == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an joypad instance");
        return status;
    }

    *value_out = joypad->reg_joyp;
    return GBSTATUS_OK;
}

gbstatus_e joypad_joyp_write(gb_joypad_t *joypad, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (joypad == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an joypad instance");
        return status;
    }

    // Only P14 and P15 are needed actually
    joypad->reg_joyp = value;
    joypad_update_reg(joypad);

    return GBSTATUS_OK;
}

gbstatus_e joypad_update(gb_joypad_t *joypad, int new_state)
{
    gbstatus_e status = GBSTATUS_OK;

    if (joypad == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as an joypad instance");
        return status;
    }

    joypad->state = new_state;
    joypad_update_reg(joypad);

    if ((joypad->reg_joyp & 0xF) != 0xF)
        GBCHK(int_request(&joypad->gb->intr_ctrl, INT_JOYPAD));

    return GBSTATUS_OK;
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