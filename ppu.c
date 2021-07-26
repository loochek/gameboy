#include "ppu.h"
#include "gb.h"

#define VRAM_SIZE 0x2000
#define OAM_SIZE  0xA0

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

#define STAT_STATE_BIT0         0
#define STAT_STATE_BIT1         1
#define STAT_LYC_FLAG_BIT       2
#define STAT_HBLANK_INT_BIT     3
#define STAT_VBLANK_INT_BIT     4
#define STAT_OBJ_SEARCH_INT_BIT 5
#define STAT_LYC_INT_BIT        6

#define STATE_OBJ_SEARCH_DURATION 80
#define STATE_DRAWING_DURATION    172
#define STATE_HBLANK_DURATION     200
#define STATE_HBLANK_INC_DURATION 4
#define STATE_VBLANK_DURATION     452
#define STATE_VBLANK_INC_DURATION 4

#define STATE_VBLANK_LAST_LINE_DURATION 456

#define SET_BIT(var, bit, val) (var = (var) & (~(1 << bit)) | ((val & 0x1) << bit))
#define GET_BIT(val, bit) ((val >> bit) & 0x1)

gbstatus_e ppu_init(gb_ppu_t *ppu, struct gb *gb)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    ppu->gb = gb;

    ppu->vram = calloc(VRAM_SIZE, sizeof(uint8_t));
    if (ppu->vram == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler0;
    }

    ppu->oam = calloc(OAM_SIZE, sizeof(uint8_t));
    if (ppu->oam == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler1;
    }

    ppu->framebuffer = calloc(SCREEN_HEIGHT * SCREEN_HEIGHT, sizeof(char));
    if (ppu->oam == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler2;
    }

    status = ppu_reset(ppu);
    if (status != GBSTATUS_OK)
        goto error_handler3;

    return GBSTATUS_OK;

error_handler3:
    free(ppu->framebuffer);

error_handler2:
    free(ppu->oam);

error_handler1:
    free(ppu->vram);

error_handler0:
    return status;
}

gbstatus_e ppu_reset(gb_ppu_t *ppu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    ppu->reg_lcdc = 0x91;
    ppu->reg_stat = 0x85;
    ppu->reg_ly   = 0x00;
    ppu->reg_lyc  = 0x00;

    return GBSTATUS_OK;
}

gbstatus_e ppu_update(gb_ppu_t *ppu, int elapsed_cycles)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    gb_t *gb = ppu->gb;

    ppu->cycles_counter += elapsed_cycles;

    while (ppu->cycles_counter >= ppu->clocks_to_next_state)
    {
        ppu->cycles_counter -= ppu->clocks_to_next_state;

        switch (ppu->curr_state)
        {
        case STATE_OBJ_SEARCH:
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 0);
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);

            if (GET_BIT(ppu->reg_stat, STAT_OBJ_SEARCH_INT_BIT) && !ppu->lcdc_blocked)
            {
                int_request(gb->intr_ctrl, INT_LCDC);
                ppu->lcdc_blocked = true;
            }

            ppu->curr_state = STATE_DRAWING;
            ppu->clocks_to_next_state = STATE_OBJ_SEARCH_DURATION;
            break;

        case STATE_DRAWING:
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 1);
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 1);

            ppu->curr_state = STATE_HBLANK;
            ppu->clocks_to_next_state = STATE_DRAWING_DURATION;
            break;

        case STATE_HBLANK:
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 0);
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);

            if (GET_BIT(ppu->reg_stat, STAT_HBLANK_INT_BIT) && !ppu->lcdc_blocked)
            {
                GBCHK(int_request(gb->intr_ctrl, INT_LCDC));
                ppu->lcdc_blocked = true;
            }

            ppu->curr_state = STATE_HBLANK_INC;
            ppu->clocks_to_next_state = STATE_HBLANK_DURATION;
            break;

        case STATE_HBLANK_INC:
            ppu->reg_ly++;
            ppu->lcdc_blocked = false;

            if (ppu->reg_ly == ppu->reg_lyc)
            {
                SET_BIT(ppu->reg_stat, STAT_LYC_FLAG_BIT, 1);
                GBCHK(int_request(gb->intr_ctrl, INT_LCDC));
                ppu->lcdc_blocked = true;
            }
            else
                SET_BIT(ppu->reg_stat, STAT_LYC_FLAG_BIT, 0);

            if (ppu->reg_ly >= SCREEN_HEIGHT)
                ppu->curr_state = STATE_VBLANK;
            else
                ppu->curr_state = STATE_OBJ_SEARCH;

            ppu->clocks_to_next_state = STATE_HBLANK_INC_DURATION;
            break;

        case STATE_VBLANK:
            if (ppu->reg_ly == SCREEN_HEIGHT)
            {
                SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 1);
                SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);

                GBCHK(int_request(gb->intr_ctrl, INT_VBLANK));
            }

            if (GET_BIT(ppu->reg_stat, STAT_VBLANK_INT_BIT) && !ppu->lcdc_blocked)
            {
                GBCHK(int_request(gb->intr_ctrl, INT_LCDC));
                ppu->lcdc_blocked = true;
            }

            ppu->curr_state = STATE_VBLANK_INC;
            ppu->clocks_to_next_state = STATE_VBLANK_DURATION;
            break;

        case STATE_VBLANK_INC:
            ppu->reg_ly++;
            ppu->lcdc_blocked = false;

            if (ppu->reg_ly == ppu->reg_lyc)
            {
                SET_BIT(ppu->reg_stat, STAT_LYC_FLAG_BIT, 1);
                GBCHK(int_request(gb->intr_ctrl, INT_LCDC));
                ppu->lcdc_blocked = true;
            }
            else
                SET_BIT(ppu->reg_stat, STAT_LYC_FLAG_BIT, 0);

            if (ppu->reg_ly >= SCREEN_HEIGHT + 10 - 1)
                ppu->curr_state = STATE_VBLANK_LAST_LINE;
            else
                ppu->curr_state = STATE_VBLANK;

            ppu->clocks_to_next_state = STATE_VBLANK_INC_DURATION;
            break;

        case STATE_VBLANK_LAST_LINE:
            ppu->reg_ly = 0;
            ppu->lcdc_blocked = false;

            ppu->curr_state = STATE_OBJ_SEARCH;
            ppu->clocks_to_next_state = STATE_VBLANK_LAST_LINE_DURATION;
            break;
        } 
    }

    return GBSTATUS_OK;
}

gbstatus_e ppu_lcdc_read(gb_ppu_t *ppu, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    *value_out = ppu->reg_lcdc;
    return GBSTATUS_OK;
}

gbstatus_e ppu_stat_read(gb_ppu_t *ppu, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    *value_out = ppu->reg_stat;
    return GBSTATUS_OK;
}

gbstatus_e ppu_ly_read(gb_ppu_t *ppu, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    *value_out = ppu->reg_ly;
    return GBSTATUS_OK;
}

gbstatus_e ppu_lyc_read(gb_ppu_t *ppu, uint8_t *value_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    *value_out = ppu->reg_lyc;
    return GBSTATUS_OK;
}

gbstatus_e ppu_vram_read(gb_ppu_t *ppu, uint16_t addr, uint8_t *byte_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    *byte_out = 0xFF;
    return GBSTATUS_OK;
}

gbstatus_e ppu_oam_read(gb_ppu_t *ppu, uint16_t addr, uint8_t *byte_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    *byte_out = 0xFF;
    return GBSTATUS_OK;
}

gbstatus_e ppu_lcdc_write(gb_ppu_t *ppu, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    ppu->reg_lcdc = value;
    return GBSTATUS_OK;
}

gbstatus_e ppu_stat_write(gb_ppu_t *ppu, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    ppu->reg_stat = (ppu->reg_stat & 0x7) | (value & 0x78);
    return GBSTATUS_OK;
}

gbstatus_e ppu_ly_write(gb_ppu_t *ppu, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    // writing LY is not allowed
    return GBSTATUS_OK;
}

gbstatus_e ppu_lyc_write(gb_ppu_t *ppu, uint8_t value)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    ppu->reg_lyc = value;
    return GBSTATUS_OK;
}

gbstatus_e ppu_vram_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    return GBSTATUS_OK;
}

gbstatus_e ppu_oam_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    return GBSTATUS_OK;
}

gbstatus_e ppu_deinit(gb_ppu_t *ppu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (ppu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as PPU instance");
        return status;
    }

    free(ppu->vram);
    free(ppu->oam);
    free(ppu->framebuffer);
    return GBSTATUS_OK;
}