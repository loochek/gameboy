#include "mbc1.h"
#include "cart.h"

/**
 * Check this out to understand logic:
 * https://gbdev.io/pandocs/MBC1.html
 */

typedef struct
{
    bool ram_enabled;

    /// If true, mode 1 of MBC1 is used
    bool second_mode;
} mbc1_state_t;

gbstatus_e mbc1_init(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    cart->mbc_state = calloc(1, sizeof(mbc1_state_t));
    if (cart->mbc_state == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        return status;
    }

    return GBSTATUS_OK;
}

gbstatus_e mbc1_reset(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    mbc1_state_t *state = (mbc1_state_t*)cart->mbc_state;
    state->ram_enabled = false;
    state->second_mode = false;
    cart->curr_rom_bank = 1;

    return GBSTATUS_OK;
}

gbstatus_e mbc1_read(gb_cart_t *cart, uint16_t addr, uint8_t *byte_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    mbc1_state_t *state = (mbc1_state_t*)cart->mbc_state;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
        // ROM bank 0 in mode 0
        *byte_out = cart->rom[addr];
        break;

    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
    {
        // Switchable ROM bank
        int offset = cart->curr_rom_bank * ROM_BANK_SIZE;
        *byte_out = cart->rom[offset + addr - 0x4000];
        break;
    }
    
    case 0xA000:
    case 0xB000:
        // External RAM
        if (state->ram_enabled)
        {
            int offset = 0;
            if (!state->second_mode)
                offset = cart->curr_ram_bank * SRAM_BANK_SIZE;

            *byte_out = cart->ram[offset + addr - 0xA000];
        }
        break;
    
    default:
        *byte_out = 0xFF;
        break;
    }

    return GBSTATUS_OK;
}

gbstatus_e mbc1_write(struct gb_cart *cart, uint16_t addr, uint8_t byte)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    mbc1_state_t *state = (mbc1_state_t*)cart->mbc_state;

    switch (addr & 0xF000)
    {
    case 0x0000:
    case 0x1000:
        // RAM enable
        state->ram_enabled = (byte & 0xF) == 0xA;
        break;

    case 0x2000:
    case 0x3000:
        // ROM bank number
        cart->curr_rom_bank = (cart->curr_rom_bank & (~0x1F)) | (byte & 0x1F);
        cart->curr_rom_bank %= cart->rom_size;

        switch (cart->curr_rom_bank)
        {
        case 0x00:
            cart->curr_rom_bank = 0x01;
            break;
        
        case 0x20:
            cart->curr_rom_bank = 0x21;
            break;

        case 0x40:
            cart->curr_rom_bank = 0x41;
            break;

        case 0x60:
            cart->curr_rom_bank = 0x61;
            break;

        default:
            break;
        }

        break;

    case 0x4000:
    case 0x5000:
        //  RAM Bank Number (mode 0) or Upper Bits of ROM Bank Number (mode 1)
        if (state->second_mode)
        {
            cart->curr_rom_bank = (cart->curr_rom_bank & 0x1F) | ((byte & 0x3) << 5);
            cart->curr_rom_bank %= cart->rom_size;

            switch (cart->curr_rom_bank)
            {
            case 0x00:
                cart->curr_rom_bank = 0x01;
                break;
            
            case 0x20:
                cart->curr_rom_bank = 0x21;
                break;

            case 0x40:
                cart->curr_rom_bank = 0x41;
                break;

            case 0x60:
                cart->curr_rom_bank = 0x61;
                break;

            default:
                break;
            }
        }
        else
        {
            cart->curr_ram_bank = byte & 0x3;
            cart->curr_ram_bank %= cart->ram_size;
        }

        break;

    case 0x6000:
    case 0x7000:
        // Mode switching
        state->second_mode = byte & 0x1;
        break;

    case 0xA000:
    case 0xB000:
        // External RAM
        if (state->ram_enabled)
        {
            int offset = 0;
            if (!state->second_mode)
                offset = cart->curr_ram_bank * SRAM_BANK_SIZE;

            cart->ram[offset + addr - 0xA000] = byte;
        }
        break;
    
    default:
        break;
    }

    return GBSTATUS_OK;
}

gbstatus_e mbc1_deinit(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    free(cart->mbc_state);
    return GBSTATUS_OK;
}