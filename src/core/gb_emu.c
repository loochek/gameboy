#include <assert.h>
#include "gb_emu.h"

gbstatus_e gb_emu_init(gb_emu_t *gb_emu)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(gb_emu != NULL);

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

    gb_emu->cart_inserted = false;
    return GBSTATUS_OK;

error_handler2:
    GBCHK(ppu_deinit(&gb->ppu));

error_handler1:
    GBCHK(mmu_deinit(&gb->mmu));

error_handler0:
    return status;
}

gbstatus_e gb_emu_framebuffer_ptr(gb_emu_t *gb_emu, const char **fb_ptr_out)
{
    assert(gb_emu != NULL);
    assert(fb_ptr_out != NULL);

    *fb_ptr_out = gb_emu->gb.ppu.framebuffer;
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_frame_ready_ptr(gb_emu_t *gb_emu, const bool **ready_ptr_out)
{
    assert(gb_emu != NULL);
    assert(ready_ptr_out != NULL);

    *ready_ptr_out = &gb_emu->gb.ppu.new_frame_ready;
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_grab_frame(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    gb_emu->gb.ppu.new_frame_ready = false;
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_game_title_ptr(gb_emu_t *gb_emu, const char **title_out)
{
    assert(gb_emu != NULL);
    assert(title_out != NULL);

    if (gb_emu->cart_inserted)
        *title_out = gb_emu->cart.game_title;
    else
        *title_out = NULL;

    return GBSTATUS_OK;
}

gbstatus_e gb_emu_change_rom(gb_emu_t *gb_emu, const char *rom_file_path)
{
    assert(gb_emu != NULL);
    assert(rom_file_path != NULL);

    if (gb_emu->cart_inserted)
        GBCHK(cart_deinit(&gb_emu->cart));

    GBCHK(cart_init(&gb_emu->cart, rom_file_path));
    GBCHK(mmu_switch_cart(&gb_emu->gb.mmu, &gb_emu->cart));
    GBCHK(gb_emu_reset(gb_emu));

    gb_emu->cart_inserted = true;
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_unload_rom(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    if (gb_emu->cart_inserted)
        GBCHK(cart_deinit(&gb_emu->cart));

    GBCHK(mmu_switch_cart(&gb_emu->gb.mmu, NULL));
    GBCHK(gb_emu_reset(gb_emu));

    gb_emu->cart_inserted = false;
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_reset(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    gb_t *gb = &gb_emu->gb;

    GBCHK(cpu_reset(&gb->cpu));
    GBCHK(mmu_reset(&gb->mmu));
    GBCHK(ppu_reset(&gb->ppu));
    GBCHK(int_reset(&gb->intr_ctrl));
    GBCHK(timer_reset(&gb->timer));
    GBCHK(joypad_reset(&gb->joypad));

    // MMU resets inserted cartridge

    return GBSTATUS_OK;
}

gbstatus_e gb_emu_step(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    GBCHK(cpu_step(&gb_emu->gb.cpu));
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_update_input(gb_emu_t *gb_emu, int new_state)
{
    assert(gb_emu != NULL);

    GBCHK(joypad_update(&gb_emu->gb.joypad, new_state));
    return GBSTATUS_OK;
}

gbstatus_e gb_emu_deinit(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    gb_t *gb = &gb_emu->gb;

    if (gb_emu->cart_inserted)
        GBCHK(cart_deinit(&gb_emu->cart));

    GBCHK(ppu_deinit(&gb->ppu));
    GBCHK(mmu_deinit(&gb->mmu));

    return GBSTATUS_OK;
}