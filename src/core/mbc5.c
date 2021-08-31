#include <assert.h>
#include "mbc5.h"
#include "cart.h"
#include "log.h"

/**
 * Check this out to understand logic:
 * https://gbdev.io/pandocs/MBC5.html
 */

typedef struct
{
    bool ram_enabled;
} mbc5_state_t;

gbstatus_e mbc5_init(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(cart != NULL);

    cart->mbc_state = calloc(1, sizeof(mbc5_state_t));
    if (cart->mbc_state == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        return status;
    }

    return GBSTATUS_OK;
}

void mbc5_reset(gb_cart_t *cart)
{
    assert(cart != NULL);

    mbc5_state_t *state = (mbc5_state_t*)cart->mbc_state;
    state->ram_enabled = false;
    cart->curr_rom_bank = 1;
    cart->curr_ram_bank = 0;
}

uint8_t mbc5_read(gb_cart_t *cart, uint16_t addr)
{
    assert(cart != NULL);

    mbc5_state_t *state = (mbc5_state_t*)cart->mbc_state;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
        // ROM bank 0 in mode 0
        return cart->rom[addr];

    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
    {
        // Switchable ROM bank
        int offset = cart->curr_rom_bank * ROM_BANK_SIZE;
        return cart->rom[offset + addr - 0x4000];
    }
    
    case 0xA000:
    case 0xB000:
        // External RAM
        if (state->ram_enabled)
        {
            int offset = cart->curr_ram_bank * SRAM_BANK_SIZE;
            return cart->ram[offset + addr - 0xA000];
        }
        else
            return 0xFF;
    
    default:
        return 0xFF;
    }
}

void mbc5_write(struct gb_cart *cart, uint16_t addr, uint8_t byte)
{
    assert(cart != NULL);

    mbc5_state_t *state = (mbc5_state_t*)cart->mbc_state;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
        // RAM enable
        state->ram_enabled = (byte & 0xF) == 0xA;
        break;

    case 0x2000:
        // 8 least significant bits of ROM bank number
        cart->curr_rom_bank = (cart->curr_rom_bank & (~0xFF)) | (byte & 0xFF);
        cart->curr_rom_bank %= cart->rom_size;

        break;

    case 0x3000:
        // 9th bit of ROM bank number
        cart->curr_rom_bank = (cart->curr_rom_bank & (~0x100)) | ((byte & 0x1) << 8);
        cart->curr_rom_bank %= cart->rom_size;

        break;

    case 0x4000:
    case 0x5000:
        // RAM bank number
        cart->curr_ram_bank = byte;
        cart->curr_ram_bank %= cart->ram_size;

        break;

    default:
        break;
    }
}

void mbc5_deinit(gb_cart_t *cart)
{
    assert(cart != NULL);
    
    free(cart->mbc_state);
}