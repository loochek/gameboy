#include "gb_emu.h"

gbstatus_e gb_emu_init(gb_emu_t *gb_emu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (gb_emu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as emulator instance");
        return status;
    }

    gb_t *gb = &gb_emu->gb;

    status = cpu_init(&gb->cpu, gb);
    if (status != GBSTATUS_OK)
        goto error_handler0;

    status = int_init(&gb->intr_ctrl, gb);
    if (status != GBSTATUS_OK)
        goto error_handler0;

    status = timer_init(&gb->timer, gb);
    if (status != GBSTATUS_OK)
        goto error_handler0;

    status = joypad_init(&gb->joypad, gb);
    if (status != GBSTATUS_OK)
        goto error_handler0;

    status = mmu_init(&gb->mmu, gb);
    if (status != GBSTATUS_OK)
        goto error_handler1;

    status = ppu_init(&gb->ppu, gb);
    if (status != GBSTATUS_OK)
        goto error_handler2;

    gb_emu->cart_inserted   = false;
    gb_emu->new_frame_ready = false;
    gb_emu->framebuffer     = gb->ppu.framebuffer;
    gb_emu->game_title      = NULL;
    return GBSTATUS_OK;

error_handler2:
    GBCHK(ppu_deinit(&gb->ppu));

error_handler1:
    GBCHK(mmu_deinit(&gb->mmu));

error_handler0:
    return status;
}

gbstatus_e gb_emu_change_rom(gb_emu_t *gb_emu, const char *rom_file_path)
{
    gbstatus_e status = GBSTATUS_OK;

    if (gb_emu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as emulator instance");
        return status;
    }

    if (gb_emu->cart_inserted)
        GBCHK(cart_deinit(&gb_emu->cart));

    GBCHK(cart_init(&gb_emu->cart, rom_file_path));
    GBCHK(mmu_switch_cart(&gb_emu->gb.mmu, &gb_emu->cart));
    GBCHK(gb_emu_reset(gb_emu));

    gb_emu->cart_inserted = true;
    gb_emu->game_title    = gb_emu->cart.game_title;
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_reset(gb_emu_t *gb_emu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (gb_emu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as emulator instance");
        return status;
    }

    gb_t *gb = &gb_emu->gb;

    GBCHK(cpu_reset(&gb->cpu));
    GBCHK(mmu_reset(&gb->mmu));
    GBCHK(ppu_reset(&gb->ppu));
    GBCHK(int_reset(&gb->intr_ctrl));
    GBCHK(timer_reset(&gb->timer));
    GBCHK(joypad_reset(&gb->joypad));
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_step(gb_emu_t *gb_emu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (gb_emu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as emulator instance");
        return status;
    }

    gb_t *gb = &gb_emu->gb;

    GBCHK(cpu_step(&gb->cpu));

    if (gb->ppu.new_frame_ready)
    {
        gb_emu->new_frame_ready = true;
        gb->ppu.new_frame_ready = false;
    }
    
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_update_input(gb_emu_t *gb_emu, int new_state)
{
    gbstatus_e status = GBSTATUS_OK;

    if (gb_emu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as emulator instance");
        return status;
    }

    GBCHK(joypad_update(&gb_emu->gb.joypad, new_state));
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_deinit(gb_emu_t *gb_emu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (gb_emu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as emulator instance");
        return status;
    }

    gb_t *gb = &gb_emu->gb;

    if (gb_emu->cart_inserted)
        GBCHK(cart_deinit(&gb_emu->cart));

    GBCHK(ppu_deinit(&gb->ppu));
    GBCHK(mmu_deinit(&gb->mmu));

    return GBSTATUS_OK;
}