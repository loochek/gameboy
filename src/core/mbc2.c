#include <assert.h>
#include "mbc1.h"
#include "cart.h"

/**
 * Check this out to understand logic:
 * https://gbdev.io/pandocs/MBC2.html
 */

typedef struct
{
    bool ram_enabled;
} mbc2_state_t;

gbstatus_e mbc2_init(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(cart != NULL);

    cart->mbc_state = calloc(1, sizeof(mbc2_state_t));
    if (cart->mbc_state == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        return status;
    }

    return GBSTATUS_OK;
}

void mbc2_reset(gb_cart_t *cart)
{
    assert(cart != NULL);

    mbc2_state_t *state = (mbc2_state_t*)cart->mbc_state;
    state->ram_enabled = false;
    cart->curr_rom_bank = 1;
}

uint8_t mbc2_read(gb_cart_t *cart, uint16_t addr)
{
    assert(cart != NULL);

    mbc2_state_t *state = (mbc2_state_t*)cart->mbc_state;

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
            // Only 9 least significant bits of address are used, RAM access repeats
            return cart->ram[(addr - 0xA000) & 0x1FF];
        }
        else
            return 0xFF;
    
    default:
        return 0xFF;
    }
}

void mbc2_write(struct gb_cart *cart, uint16_t addr, uint8_t byte)
{
    assert(cart != NULL);

    mbc2_state_t *state = (mbc2_state_t*)cart->mbc_state;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
        if (addr & 0x100) // 8th bit is set
        {
            // ROM bank number
            cart->curr_rom_bank = byte & 0xF;
            cart->curr_rom_bank %= cart->rom_size;
            
            if (cart->curr_rom_bank == 0)
                cart->curr_rom_bank = 1;
        }
        else // 8th bit is clear
        {
            // RAM enable
            state->ram_enabled = (byte == 0xA);
        }
        
        break;

    case 0xA000:
    case 0xB000:
        // External RAM
        if (state->ram_enabled)
            cart->ram[(addr - 0xA000) & 0x1FF] = byte;

        break;
    
    default:
        break;
    }
}

void mbc2_deinit(gb_cart_t *cart)
{
    assert(cart != NULL);
    
    free(cart->mbc_state);
}