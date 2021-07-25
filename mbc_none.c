#include "mbc_none.h"
#include "cart.h"

gbstatus_e mbc_none_init(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    // Some games uses external RAM even without mapper

    cart->ram = calloc(SRAM_BANK_SIZE, sizeof(uint8_t));
    if (cart->ram == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        return status;
    }

    cart->ram_size = 1;
    return GBSTATUS_OK;
}

gbstatus_e mbc_none_reset(gb_cart_t *cart)
{
    return GBSTATUS_OK;
}

gbstatus_e mbc_none_read(gb_cart_t *cart, uint16_t addr, uint8_t *byte_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

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
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

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
        cart->rom[addr] = byte;
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
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    free(cart->ram);
    return GBSTATUS_OK;
}