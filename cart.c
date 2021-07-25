#include "cart.h"
#include "mbc_none.h"
#include "mbc1.h"

gbstatus_e cart_init(gb_cart_t *cart, const char *rom_path)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cart == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as cartridge instance");
        return status;
    }

    FILE *rom_file = fopen(rom_path, "r");
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

    uint8_t mapper = cart->rom[CART_TYPE_ADDR];
    switch (mapper)
    {
    case 0x00:
        // No mapper
        status = mbc_none_init(cart);
        if (status != GBSTATUS_OK)
            goto error_handler2;

        cart->mbc_read_func   = mbc_none_read;
        cart->mbc_write_func  = mbc_none_write;
        cart->mbc_reset_func  = mbc_none_reset;
        cart->mbc_deinit_func = mbc_none_deinit;
        break;

    case 0x01:
    case 0x02:
    case 0x03:
        // MBC1
        status = mbc1_init(cart);
        if (status != GBSTATUS_OK)
            goto error_handler2;

        cart->mbc_read_func   = mbc1_read;
        cart->mbc_write_func  = mbc1_write;
        cart->mbc_reset_func  = mbc1_reset;
        cart->mbc_deinit_func = mbc1_deinit;
        break;

    default:
        GBSTATUS(GBSTATUS_NOT_IMPLEMENTED, "unsupported mapper");
        goto error_handler2;

        break;
    }

    fclose(rom_file);
    return GBSTATUS_OK;

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

    GBCHK(cart->mbc_deinit_func(cart));
    free(cart->rom);

    return GBSTATUS_OK;
}