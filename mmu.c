#include <stdio.h>
#include <string.h>
#include "mmu.h"
#include "cart.h"
#include "gb.h"

#define RAM_SIZE  0x2000
#define HRAM_SIZE 0x100

static uint8_t gb_bootrom[] =
{
    0x31,0xFE,0xFF,0xAF,0x21,0xFF,0x9F,0x32,0xCB,0x7C,0x20,0xFB,0x21,0x26,0xFF,0x0E,
    0x11,0x3E,0x80,0x32,0xE2,0x0C,0x3E,0xF3,0xE2,0x32,0x3E,0x77,0x77,0x3E,0xFC,0xE0,
    0x47,0x11,0x04,0x01,0x21,0x10,0x80,0x1A,0xCD,0x95,0x00,0xCD,0x96,0x00,0x13,0x7B,
    0xFE,0x34,0x20,0xF3,0x11,0xD8,0x00,0x06,0x08,0x1A,0x13,0x22,0x23,0x05,0x20,0xF9,
    0x3E,0x19,0xEA,0x10,0x99,0x21,0x2F,0x99,0x0E,0x0C,0x3D,0x28,0x08,0x32,0x0D,0x20,
    0xF9,0x2E,0x0F,0x18,0xF3,0x67,0x3E,0x64,0x57,0xE0,0x42,0x3E,0x91,0xE0,0x40,0x04,
    0x1E,0x02,0x0E,0x0C,0xF0,0x44,0xFE,0x90,0x20,0xFA,0x0D,0x20,0xF7,0x1D,0x20,0xF2,
    0x0E,0x13,0x24,0x7C,0x1E,0x83,0xFE,0x62,0x28,0x06,0x1E,0xC1,0xFE,0x64,0x20,0x06,
    0x7B,0xE2,0x0C,0x3E,0x87,0xE2,0xF0,0x42,0x90,0xE0,0x42,0x15,0x20,0xD2,0x05,0x20,
    0x4F,0x16,0x20,0x18,0xCB,0x4F,0x06,0x04,0xC5,0xCB,0x11,0x17,0xC1,0xCB,0x11,0x17,
    0x05,0x20,0xF5,0x22,0x23,0x22,0x23,0xC9,0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,
    0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,
    0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,
    0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E,0x3C,0x42,0xB9,0xA5,0xB9,0xA5,0x42,0x3C,
    0x21,0x04,0x01,0x11,0xA8,0x00,0x1A,0x13,0xBE,0x20,0xFE,0x23,0x7D,0xFE,0x34,0x20,
    0xF5,0x06,0x19,0x78,0x86,0x23,0x05,0x20,0xFB,0x86,0x20,0xFE,0x3E,0x01,0xE0,0x50
};

gbstatus_e mmu_init(gb_mmu_t *mmu, gb_t *gb)
{
    gbstatus_e status = GBSTATUS_OK;

    if (mmu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as MMU instance");
        return status;
    }

    mmu->gb = gb;

    uint8_t *ram = calloc(RAM_SIZE + HRAM_SIZE, sizeof(uint8_t));
    if (ram == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler0;
    }

    mmu->ram  = ram;
    mmu->hram = ram + RAM_SIZE;

    mmu->serial_data = 0x00;
    mmu->cart = NULL;

    status = mmu_reset(mmu);
    if (status != GBSTATUS_OK)
        goto error_handler1;

    return GBSTATUS_OK;

error_handler1:
    free(ram);

error_handler0:
    return status;
}

gbstatus_e mmu_reset(gb_mmu_t *mmu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (mmu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as MMU instance");
        return status;
    }

    memset(mmu->ram , 0, RAM_SIZE);
    memset(mmu->hram, 0, HRAM_SIZE);

    if (mmu->cart != NULL)
        GBCHK(cart_reset(mmu->cart));

    mmu->bootrom_mapped = true;
    return GBSTATUS_OK;
}

gbstatus_e mmu_switch_cart(gb_mmu_t *mmu, struct gb_cart *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    if (mmu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as MMU instance");
        return status;
    }

    mmu->cart = cart;
    GBCHK(mmu_reset(mmu));
    return GBSTATUS_OK;
}

gbstatus_e mmu_read(gb_mmu_t *mmu, uint16_t addr, uint8_t *byte_out)
{
    gbstatus_e status = GBSTATUS_OK;

    if (mmu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as MMU instance");
        return status;
    }

    gb_t *gb = mmu->gb;

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
    case 0xA000:
    case 0xB000:
        if (addr < 0x100 && mmu->bootrom_mapped)
        {
            *byte_out = gb_bootrom[addr];
        }
        else
        {
            // ROM and external RAM requests are redirected to the cartridge
            if (mmu->cart == NULL)
                *byte_out = 0xFF;
            else
                GBCHK(cart_read(mmu->cart, addr, byte_out));
        }

        break;

    case 0x8000:
    case 0x9000:
        // VRAM
        GBCHK(ppu_vram_read(gb->ppu, addr, byte_out));
        break;

    case 0xC000:
    case 0xD000:
        // Internal RAM
        *byte_out = mmu->ram[addr - 0xC000];
        break;
    
    case 0xE000:
        // unused
        *byte_out = 0xFF;
        break;

    case 0xF000:
        switch (addr & 0xF00)
        {
        case 0x000:
        case 0x100:
        case 0x200:
        case 0x300:
        case 0x400:
        case 0x500:
        case 0x600:
        case 0x700:
        case 0x800:
        case 0x900:
        case 0xA00:
        case 0xB00:
        case 0xC00:
        case 0xD00:
            // unused
            *byte_out = 0xFF;
            break;
        
        case 0xE00:
            if (addr >= 0xFEA0)
                // unused
                *byte_out = 0xFF;
                break;
            
            // OAM
            GBCHK(ppu_oam_read(gb->ppu, addr, byte_out));
            break;

        case 0xF00:
            switch (addr & 0xFF)
            {
            case 0x0F:
                // IF (interrupt controller)
                GBCHK(int_if_read(gb->intr_ctrl, byte_out));
                break;

            case 0xFF:
                // IE (interrupt controller)
                GBCHK(int_ie_read(gb->intr_ctrl, byte_out));
                break;

            case 0x00:
                // JOYP (Joypad)
                GBCHK(joypad_joyp_read(gb->joypad, byte_out));
                break;

            case 0x04:
                // DIV (timer)
                GBCHK(timer_div_read(gb->timer, byte_out));
                break;

            case 0x05:
                // TIMA (timer)
                GBCHK(timer_tima_read(gb->timer, byte_out));
                break;

            case 0x06:
                // TMA (timer)
                GBCHK(timer_tma_read(gb->timer, byte_out));
                break;

            case 0x07:
                // TAC (timer)
                GBCHK(timer_tac_read(gb->timer, byte_out));
                break;

            case 0x40:
                // LCDC (PPU)
                GBCHK(ppu_lcdc_read(gb->ppu, byte_out));
                break;

            case 0x41:
                // STAT (PPU)
                GBCHK(ppu_stat_read(gb->ppu, byte_out));
                break;

            case 0x42:
                // SCY (PPU)
                GBCHK(ppu_scy_read(gb->ppu, byte_out));
                break;

            case 0x43:
                // SCX (PPU)
                GBCHK(ppu_scx_read(gb->ppu, byte_out));
                break;

            case 0x44:
                // LY (PPU)
                GBCHK(ppu_ly_read(gb->ppu, byte_out));
                break;

            case 0x45:
                // LYC (PPU)
                GBCHK(ppu_lyc_read(gb->ppu, byte_out));
                break;

            case 0x47:
                // BGP (PPU)
                GBCHK(ppu_bgp_read(gb->ppu, byte_out));
                break;

            case 0x4A:
                // WY (PPU)
                GBCHK(ppu_wy_read(gb->ppu, byte_out));
                break;

            case 0x4B:
                // WX (PPU)
                GBCHK(ppu_wx_read(gb->ppu, byte_out));
                break;

            default:
                if (addr >= 0xFF80 && addr < 0xFFFF)
                    *byte_out = mmu->hram[addr - 0xFF80];
                else
                    *byte_out = 0xFF;
                
                break;
            }

            break;
        }

        break;
    }

    return GBSTATUS_OK;
}

gbstatus_e mmu_write(gb_mmu_t *mmu, uint16_t addr, uint8_t byte)
{
    gbstatus_e status = GBSTATUS_OK;

    if (mmu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as MMU instance");
        return status;
    }

    gb_t *gb = mmu->gb;

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
    case 0xA000:
    case 0xB000:
        // ROM and external RAM requests are redirected to the cartridge
        if ((!mmu->bootrom_mapped || addr >= 0x100) && mmu->cart != NULL)
            GBCHK(cart_write(mmu->cart, addr, byte));
        break;

    case 0x8000:
    case 0x9000:
        GBCHK(ppu_vram_write(gb->ppu, addr, byte));
        break;

    case 0xC000:
    case 0xD000:
        // Internal RAM
        mmu->ram[addr - 0xC000] = byte;
        break;
    
    case 0xE000:
        // unused
        break;

    case 0xF000:
        switch (addr & 0xF00)
        {
        case 0x000:
        case 0x100:
        case 0x200:
        case 0x300:
        case 0x400:
        case 0x500:
        case 0x600:
        case 0x700:
        case 0x800:
        case 0x900:
        case 0xA00:
        case 0xB00:
        case 0xC00:
        case 0xD00:
            // unused
            break;
        
        case 0xE00:
            if (addr >= 0xFEA0)
                // unused
                break;
            
            // OAM
            GBCHK(ppu_oam_write(gb->ppu, addr, byte));
            break;

        case 0xF00:
            switch (addr & 0xFF)
            {
            case 0x0F:
                // IF (interrupt controller)
                GBCHK(int_if_write(gb->intr_ctrl, byte));
                break;

            case 0xFF:
                // IE (interrupt controller)
                GBCHK(int_ie_write(gb->intr_ctrl, byte));
                break;

            case 0x00:
                // JOYP (Joypad)
                GBCHK(joypad_joyp_write(gb->joypad, byte));
                break;

            case 0x04:
                // DIV (timer)
                GBCHK(timer_div_write(gb->timer, byte));
                break;

            case 0x05:
                // TIMA (timer)
                GBCHK(timer_tima_write(gb->timer, byte));
                break;

            case 0x06:
                // TMA (timer)
                GBCHK(timer_tma_write(gb->timer, byte));
                break;

            case 0x07:
                // TAC (timer)
                GBCHK(timer_tac_write(gb->timer, byte));
                break;

            case 0x40:
                // LCDC (PPU)
                GBCHK(ppu_lcdc_write(gb->ppu, byte));
                break;

            case 0x41:
                // STAT (PPU)
                GBCHK(ppu_stat_write(gb->ppu, byte));
                break;

            case 0x42:
                // SCY (PPU)
                GBCHK(ppu_scy_write(gb->ppu, byte));
                break;

            case 0x43:
                // SCX (PPU)
                GBCHK(ppu_scx_write(gb->ppu, byte));
                break;

            case 0x44:
                // LY (PPU)
                GBCHK(ppu_ly_write(gb->ppu, byte));
                break;

            case 0x45:
                // LYC (PPU)
                GBCHK(ppu_lyc_write(gb->ppu, byte));
                break;

            case 0x47:
                // BGP (PPU)
                GBCHK(ppu_bgp_write(gb->ppu, byte));
                break;

            case 0x4A:
                // WY (PPU)
                GBCHK(ppu_wy_write(gb->ppu, byte));
                break;

            case 0x4B:
                // WX (PPU)
                GBCHK(ppu_wx_write(gb->ppu, byte));
                break;

            case 0x50:
                // Bootrom mapping
                mmu->bootrom_mapped = false;
                break;

            case 0x01:
                // Serial transfer data
                mmu->serial_data = byte;
                break;

            case 0x02:
                // Serial transfer control
                // For debugging
                if (byte == 0x81)
                {
                    printf("%c", (char)mmu->serial_data);
                    fflush(stdout);
                    
                    mmu->serial_data = 0x00;
                }

                break;

            default:
                if (addr >= 0xFF80 && addr < 0xFFFF)
                    mmu->hram[addr - 0xFF80] = byte;
                
                break;
            }

            break;
        }

        break;
    }

    return GBSTATUS_OK;
}

gbstatus_e mmu_deinit(gb_mmu_t *mmu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (mmu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as MMU instance");
        return status;
    }

    free(mmu->ram);
    // HRAM is in the same block!
    return GBSTATUS_OK;
}