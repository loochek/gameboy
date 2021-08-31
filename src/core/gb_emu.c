#include <assert.h>
#include "gb_emu.h"

gbstatus_e gb_emu_init(gb_emu_t *gb_emu)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(gb_emu != NULL);

    gb_t *gb = &gb_emu->gb;

    cpu_init   (&gb->cpu      , gb);
    int_init   (&gb->intr_ctrl, gb);
    timer_init (&gb->timer    , gb);
    joypad_init(&gb->joypad   , gb);

    status = mmu_init(&gb->mmu, gb);
    if (status != GBSTATUS_OK)
        goto error_handler0;

    status = ppu_init(&gb->ppu, gb);
    if (status != GBSTATUS_OK)
        goto error_handler1;

    gb_emu->cart_inserted = false;
    return GBSTATUS_OK;

error_handler1:
    mmu_deinit(&gb->mmu);

error_handler0:
    return status;
}

const char *gb_emu_framebuffer_ptr(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    return gb_emu->gb.ppu.framebuffer;
}

const bool *gb_emu_frame_ready_ptr(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    return &gb_emu->gb.ppu.new_frame_ready;
}

void gb_emu_grab_frame(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    gb_emu->gb.ppu.new_frame_ready = false;
}

const char *gb_emu_game_title_ptr(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);
    
    return gb_emu->cart_inserted ? gb_emu->cart.game_title : NULL;
}

gbstatus_e gb_emu_change_rom(gb_emu_t *gb_emu, const char *rom_file_path)
{
    assert(gb_emu != NULL);
    assert(rom_file_path != NULL);

    if (gb_emu->cart_inserted)
        cart_deinit(&gb_emu->cart);

    GBCHK(cart_init(&gb_emu->cart, rom_file_path));
    mmu_switch_cart(&gb_emu->gb.mmu, &gb_emu->cart);
    gb_emu_reset(gb_emu);

    gb_emu->cart_inserted = true;
    return GBSTATUS_OK;
}

void gb_emu_unload_rom(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    if (gb_emu->cart_inserted)
        cart_deinit(&gb_emu->cart);

    mmu_switch_cart(&gb_emu->gb.mmu, NULL);
    gb_emu_reset(gb_emu);

    gb_emu->cart_inserted = false;
}

void gb_emu_reset(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    gb_t *gb = &gb_emu->gb;

    cpu_reset(&gb->cpu);
    mmu_reset(&gb->mmu);
    ppu_reset(&gb->ppu);
    int_reset(&gb->intr_ctrl);
    timer_reset(&gb->timer);
    joypad_reset(&gb->joypad);
    // MMU resets inserted cartridge
}

void gb_emu_skip_bootrom(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    gb_t *gb = &gb_emu->gb;

    cpu_skip_bootrom(&gb->cpu);
    mmu_skip_bootrom(&gb->mmu);
}

gbstatus_e gb_emu_step(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    return cpu_step(&gb_emu->gb.cpu);
}

void gb_emu_update_input(gb_emu_t *gb_emu, int new_state)
{
    assert(gb_emu != NULL);

    joypad_update(&gb_emu->gb.joypad, new_state);
}

void gb_emu_deinit(gb_emu_t *gb_emu)
{
    assert(gb_emu != NULL);

    gb_t *gb = &gb_emu->gb;

    if (gb_emu->cart_inserted)
        cart_deinit(&gb_emu->cart);

    ppu_deinit(&gb->ppu);
    mmu_deinit(&gb->mmu);
}