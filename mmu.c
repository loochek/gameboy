#include <stdio.h>
#include "mmu.h"
#include "gb.h"

#define ROM_SIZE  0x8000
#define RAM_SIZE  0x2000
#define HRAM_SIZE 0x100
#define SRAM_SIZE 0x2000

gbstatus_e mmu_init(gb_mmu_t *mmu, gb_t *gb, const char *rom_path)
{
    gbstatus_e status = GBSTATUS_OK;

    if (mmu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as MMU instance");
        return status;
    }

    mmu->gb = gb;

    FILE *rom_file = fopen(rom_path, "r");
    if (rom_file == NULL)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to open ROM file");
        goto error_handler0;
    }

    mmu->rom = calloc(ROM_SIZE, sizeof(uint8_t));
    if (mmu->rom == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler1;
    }

    int bytes_read = fread(mmu->rom, sizeof(uint8_t), ROM_SIZE, rom_file);
    if (bytes_read < ROM_SIZE)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read ROM");
        goto error_handler2;
    }
    else if (bytes_read > ROM_SIZE)
    {
        GBSTATUS(GBSTATUS_NOT_IMPLEMENTED, "only 32KB roms are currently supported");
        goto error_handler2;
    }

    uint8_t *ram = calloc(RAM_SIZE + HRAM_SIZE, sizeof(uint8_t));
    if (ram == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler2;
    }

    mmu->ram  = ram;
    mmu->hram = ram + RAM_SIZE;

    mmu->serial_data = 0x00;

    fclose(rom_file);
    return GBSTATUS_OK;

error_handler2:
    free(mmu->rom);

error_handler1:
    fclose(rom_file);

error_handler0:
    return status;
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
        // ROM
        *byte_out = mmu->rom[addr];
        break;

    case 0x8000:
    case 0x9000:
        // VRAM
        *byte_out = 0xFF;
        break;

    case 0xA000:
    case 0xB000:
        // External RAM
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
                *byte_out = gb->intr_ctrl->reg_if;
                break;

            case 0xFF:
                // IE (interrupt controller)
                *byte_out = gb->intr_ctrl->reg_ie;
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
        // ROM
        break;

    case 0x8000:
    case 0x9000:
        // VRAM
        break;

    case 0xA000:
    case 0xB000:
        // External RAM
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
                gb->intr_ctrl->reg_if = byte;
                break;

            case 0xFF:
                // IE (interrupt controller)
                gb->intr_ctrl->reg_ie = byte;
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

    // FILE *sram_file = fopen("sram.bin", "wb");
    // fwrite(mmu->sram, sizeof(uint8_t), SRAM_SIZE, sram_file);
    // fclose(sram_file);

    free(mmu->rom);
    free(mmu->ram);
    // HRAM is in the same block!
    return GBSTATUS_OK;
}