#include <string.h>
#include <assert.h>
#include <errno.h>
#include "log.h"
#include "cart.h"
#include "mbc_none.h"
#include "mbc1.h"
#include "mbc2.h"
#include "mbc5.h"

#define GAME_TITLE_ADDR 0x134
#define CART_TYPE_ADDR  0x147
#define ROM_SIZE_ADDR   0x148
#define RAM_SIZE_ADDR   0x149

static gbstatus_e cart_load_sram(gb_cart_t *cart);
static gbstatus_e cart_save_sram(gb_cart_t *cart);

gbstatus_e cart_init(gb_cart_t *cart, const char *rom_path)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(cart != NULL);
    assert(rom_path != NULL);

    FILE *rom_file = fopen(rom_path, "rb");
    if (rom_file == NULL)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to open ROM file: %s", strerror(errno));
        goto error_handler0;
    }

    if (fseek(rom_file, 0, SEEK_END) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read ROM");
        goto error_handler1;
    }

    int rom_file_size = 0;
    if ((rom_file_size = ftell(rom_file)) == -1)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read ROM");
        goto error_handler1;
    }

    if (fseek(rom_file, 0, SEEK_SET) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read ROM");
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
        // Allocate some fallback SRAM anyway (also MBC2 relies on it!)
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

    case 0x05:
    case 0x06:
        // MBC2(+BATTERY)
        status = mbc2_init(cart);
        if (status != GBSTATUS_OK)
            goto error_handler3;

        cart->mbc_read_func   = mbc2_read;
        cart->mbc_write_func  = mbc2_write;
        cart->mbc_reset_func  = mbc2_reset;
        cart->mbc_deinit_func = mbc2_deinit;

        if (mapper == 0x06)
            cart->battery_backed = true;

        break;

    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        // MBC5(+RUMBLE(+RAM(+BATTERY)))
        status = mbc5_init(cart);
        if (status != GBSTATUS_OK)
            goto error_handler3;

        cart->mbc_read_func   = mbc5_read;
        cart->mbc_write_func  = mbc5_write;
        cart->mbc_reset_func  = mbc5_reset;
        cart->mbc_deinit_func = mbc5_deinit;

        if (mapper == 0x1B || mapper == 0x1E)
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
        // Try to load SRAM dump
        status = cart_load_sram(cart);
        if (status != GBSTATUS_OK)
        {
            GBSTATUS_LOG(LOG_INFO, "Failed to load SRAM dump");
            status = GBSTATUS_OK;
        }
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

void cart_reset(gb_cart_t *cart)
{
    assert(cart != NULL);

    cart->mbc_reset_func(cart);
}

uint8_t cart_read(gb_cart_t *cart, uint16_t addr)
{
    assert(cart != NULL);

    return cart->mbc_read_func(cart, addr);
}

void cart_write(gb_cart_t *cart, uint16_t addr, uint8_t byte)
{
    assert(cart != NULL);

    cart->mbc_write_func(cart, addr, byte);
}

void cart_deinit(gb_cart_t *cart)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(cart != NULL);

    if (cart->battery_backed)
    {
        status = cart_save_sram(cart);
        if (status != GBSTATUS_OK)
        {
            GBSTATUS_LOG(LOG_WARN, "Failed to save SRAM dump");
            status = GBSTATUS_OK;
        }
    }

    cart->mbc_deinit_func(cart);
    free(cart->ram);
    free(cart->rom);
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
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to open SRAM dump file: %s", strerror(errno));
        goto error_handler0;
    }

    if (fseek(save_file, 0, SEEK_END) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read SRAM dump file");
        goto error_handler1;
    }

    int save_file_size = 0;
    if ((save_file_size = ftell(save_file)) == -1)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read SRAM dump file");
        goto error_handler1;
    }

    if (fseek(save_file, 0, SEEK_SET) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read SRAM dump file");
        goto error_handler1;
    }

    if (save_file_size != cart->ram_size * SRAM_BANK_SIZE)
    {
        GBSTATUS(GBSTATUS_CART_FAIL, "SRAM dump file size is different from cartridge SRAM size");
        goto error_handler1;
    }

    int bytes_read = fread(cart->ram, sizeof(uint8_t), save_file_size, save_file);
    if (bytes_read != save_file_size)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to read SRAM dump file");
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
        GBSTATUS(GBSTATUS_IO_FAIL, "unable to open SRAM dump file: %s", strerror(errno));
        return status;
    }

    int bytes_written = fwrite(cart->ram, sizeof(uint8_t), cart->ram_size * SRAM_BANK_SIZE, save_file);
    if (bytes_written != cart->ram_size * SRAM_BANK_SIZE)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to write SRAM dump file");
        return status;
    }
    
    if (fclose(save_file) != 0)
    {
        GBSTATUS(GBSTATUS_IO_FAIL, "failed to write SRAM dump file");
        return status;
    }

    return GBSTATUS_OK;
}