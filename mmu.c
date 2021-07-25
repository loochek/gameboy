#include <stdio.h>
#include <string.h>
#include "mmu.h"
#include "cart.h"
#include "gb.h"

#define RAM_SIZE  0x2000
#define HRAM_SIZE 0x100

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
        // ROM and external RAM requests are redirected to the cartridge
        if (mmu->cart == NULL)
            *byte_out = 0xFF;
        else
            GBCHK(cart_read(mmu->cart, addr, byte_out));
        break;

    case 0x8000:
    case 0x9000:
        // VRAM
        *byte_out = 0xFF;
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
            *byte_out = 0xFF;
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
        if (mmu->cart != NULL)
            GBCHK(cart_write(mmu->cart, addr, byte));
        break;

    case 0x8000:
    case 0x9000:
        // VRAM
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