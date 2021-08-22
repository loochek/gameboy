#include <assert.h>
#include "mbc_none.h"
#include "cart.h"

gbstatus_e mbc_none_init(gb_cart_t *cart)
{
    assert(cart != NULL);
    return GBSTATUS_OK;
}

gbstatus_e mbc_none_reset(gb_cart_t *cart)
{
    assert(cart != NULL);
    return GBSTATUS_OK;
}

gbstatus_e mbc_none_read(gb_cart_t *cart, uint16_t addr, uint8_t *byte_out)
{
    assert(cart != NULL);
    assert(byte_out != NULL);

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        // ROM
        *byte_out = cart->rom[addr];
        break;

    case 0xA000:
    case 0xB000:
        // External RAM
        *byte_out = cart->ram[addr - 0xA000];
        break;
    
    default:
        *byte_out = 0xFF;
        break;
    }

    return GBSTATUS_OK;
}

gbstatus_e mbc_none_write(struct gb_cart *cart, uint16_t addr, uint8_t byte)
{
    assert(cart != NULL);

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        // ROM writing is not allowed
        break;

    case 0xA000:
    case 0xB000:
        // External RAM
        cart->ram[addr - 0xA000] = byte;
        break;
    
    default:
        break;
    }

    return GBSTATUS_OK;
}

gbstatus_e mbc_none_deinit(gb_cart_t *cart)
{
    assert(cart != NULL);
    return GBSTATUS_OK;
}