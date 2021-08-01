#include <string.h>
#include "cart.h"
#include "mbc_none.h"
#include "mbc1.h"

#define GAME_TITLE_ADDR 0x134
#define CART_TYPE_ADDR  0x147
#define ROM_SIZE_ADDR   0x148
#define RAM_SIZE_ADDR   0x149

static gbstatus_e cart_load_sram(gb_cart_t *cart);
static gbstatus_e cart_save_sram(gb_cart_t *cart);

gbstatus_e cart_init(gb_cart_t *cart, const char *rom_path)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    FILE *rom_file = fopen(rom_path, "rb");
    if (rom_file == NULL)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to open ROM file");
        goto error_handler0;
    }

    if (fseek(rom_file, 0, SEEK_END) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "fseek fail");
        goto error_handler1;
    }

    int rom_file_size = 0;
    if ((rom_file_size = ftell(rom_file)) == -1)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "ftell fail");
        goto error_handler1;
    }

    if (fseek(rom_file, 0, SEEK_SET) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "fseek fail");
        goto error_handler1;
    }
    
    if (rom_file_size < ROM_BANK_SIZE * 2)
    {
        GBSTATUS(GBSTATUS_CART_FAIL, "ROM cannot be less than 32KB");
        goto error_handler1;
    }

    cart->rom = calloc(rom_file_size, sizeof(uint8_t));
    if (cart->rom == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler1;
    }

    int bytes_read = fread(cart->rom, sizeof(uint8_t), rom_file_size, rom_file);
    if (bytes_read != rom_file_size)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read ROM");
        goto error_handler2;
    }

    int rom_size_header = (2 << cart->rom[ROM_SIZE_ADDR]) * ROM_BANK_SIZE;
    
    if (bytes_read != rom_size_header)
    {
        GBSTATUS(GBSTATUS_CART_FAIL, "ROM file size is different from header info");
        goto error_handler2;
    }

    cart->rom_size = rom_size_header / ROM_BANK_SIZE;

    uint8_t ram_size_header = cart->rom[RAM_SIZE_ADDR];
    switch (ram_size_header)
    {
    case 0x03:
        cart->ram_size = 4;
        break;

    case 0x04:
        cart->ram_size = 16;
        break;

    case 0x05:
        cart->ram_size = 8;
        break;

    default:
        // Allocate some fallback SRAM anyway
        cart->ram_size = 1;
        break;
    }

    cart->ram = calloc(cart->ram_size * SRAM_BANK_SIZE, sizeof(uint8_t));
    if (cart->ram == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler2;
    }

    cart->battery_backed = false;

    uint8_t mapper = cart->rom[CART_TYPE_ADDR];
    switch (mapper)
    {
    case 0x00:
        // No mapper
        status = mbc_none_init(cart);
        if (status != GBSTATUS_OK)
            goto error_handler3;

        cart->mbc_read_func   = mbc_none_read;
        cart->mbc_write_func  = mbc_none_write;
        cart->mbc_reset_func  = mbc_none_reset;
        cart->mbc_deinit_func = mbc_none_deinit;
        break;

    case 0x01:
    case 0x02:
    case 0x03:
        // MBC1(+RAM(+BATTERY))
        status = mbc1_init(cart);
        if (status != GBSTATUS_OK)
            goto error_handler3;

        cart->mbc_read_func   = mbc1_read;
        cart->mbc_write_func  = mbc1_write;
        cart->mbc_reset_func  = mbc1_reset;
        cart->mbc_deinit_func = mbc1_deinit;

        if (mapper == 0x03)
            cart->battery_backed = true;

        break;

    default:
        GBSTATUS(GBSTATUS_NOT_IMPLEMENTED, "unsupported mapper");
        goto error_handler3;

        break;
    }

    strncpy(cart->rom_file_path, rom_path, MAX_ROM_PATH_LEN);

    strncpy(cart->game_title, (char*)&cart->rom[GAME_TITLE_ADDR], GAME_TITLE_LEN);
    cart->game_title[GAME_TITLE_LEN] = '\0';

    if (cart->battery_backed)
    {
        // Try to load SRAM save
        // Ignore if failed
        cart_load_sram(cart);
    }

    fclose(rom_file);
    return GBSTATUS_OK;

error_handler3:
    free(cart->ram);

error_handler2:
    free(cart->rom);

error_handler1:
    fclose(rom_file);

error_handler0:
    return status;
}

gbstatus_e cart_reset(gb_cart_t *cart)
{
    return cart->mbc_reset_func(cart);
}

gbstatus_e cart_read(gb_cart_t *cart, uint16_t addr, uint8_t *byte_out)
{
    return cart->mbc_read_func(cart, addr, byte_out);
}

gbstatus_e cart_write(gb_cart_t *cart, uint16_t addr, uint8_t byte)
{
    return cart->mbc_write_func(cart, addr, byte);
}

gbstatus_e cart_deinit(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    if (cart->battery_backed)
    {
        status = cart_save_sram(cart);
        if (status != GBSTATUS_OK)
            GBSTATUS_WARN_PRINT("Unable to save");

        status = GBSTATUS_OK;
    }

    GBCHK(cart->mbc_deinit_func(cart));
    free(cart->ram);
    free(cart->rom);

    return GBSTATUS_OK;
}

static gbstatus_e cart_load_sram(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    char save_path[MAX_ROM_PATH_LEN + 10] = {0};
    strncpy(save_path, cart->rom_file_path, MAX_ROM_PATH_LEN);
    strcat(save_path, ".sav");

    FILE *save_file = fopen(save_path, "rb");
    if (save_file == NULL)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to open save file");
        goto error_handler0;
    }

    if (fseek(save_file, 0, SEEK_END) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "fseek fail");
        goto error_handler1;
    }

    int save_file_size = 0;
    if ((save_file_size = ftell(save_file)) == -1)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "ftell fail");
        goto error_handler1;
    }

    if (fseek(save_file, 0, SEEK_SET) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "fseek fail");
        goto error_handler1;
    }

    if (save_file_size != cart->ram_size * SRAM_BANK_SIZE)
    {
        GBSTATUS(GBSTATUS_CART_FAIL, "SRAM dump size is different from cartridge SRAM size");
        goto error_handler1;
    }

    int bytes_read = fread(cart->ram, sizeof(uint8_t), save_file_size, save_file);
    if (bytes_read != save_file_size)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read SRAM dump");
        goto error_handler1;
    }

    return GBSTATUS_OK;

error_handler1:
    fclose(save_file);

error_handler0:
    return status;
}

static gbstatus_e cart_save_sram(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    char save_path[MAX_ROM_PATH_LEN + 10] = {0};
    strncpy(save_path, cart->rom_file_path, MAX_ROM_PATH_LEN);
    strcat(save_path, ".sav");

    FILE *save_file = fopen(save_path, "wb");
    if (save_file == NULL)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to open file");
        return status;
    }

    int bytes_written = fwrite(cart->ram, sizeof(uint8_t), cart->ram_size * SRAM_BANK_SIZE, save_file);
    if (bytes_written != cart->ram_size * SRAM_BANK_SIZE)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to write SRAM dump");
        return status;
    }
    
    if (fclose(save_file) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to write SRAM dump");
        return status;
    }

    return GBSTATUS_OK;
}