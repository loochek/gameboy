#ifndef CART_H
#define CART_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "gbstatus.h"

struct gb_cart;

#define MAX_ROM_PATH_LEN 100
#define GAME_TITLE_LEN   16

#define ROM_BANK_SIZE  0x4000
#define SRAM_BANK_SIZE 0x2000

typedef uint8_t (*cart_read_func_t )(struct gb_cart *cart, uint16_t addr);
typedef void    (*cart_write_func_t)(struct gb_cart *cart, uint16_t addr, uint8_t byte);
typedef void    (*cart_misc_func_t )(struct gb_cart *cart);

/**
 * Represents Gameboy cartridge. 
 */
typedef struct gb_cart
{
    /// Cartridge ROM banks
    uint8_t *rom;

    /// Cartridge RAM banks
    uint8_t *ram;

    int curr_rom_bank;
    int curr_ram_bank;

    /// ROM size in banks
    int rom_size;

    /// RAM size in banks
    int ram_size;

    bool battery_backed;

    char rom_file_path[MAX_ROM_PATH_LEN + 1];

    char game_title[GAME_TITLE_LEN + 1];

    cart_read_func_t  mbc_read_func;
    cart_write_func_t mbc_write_func;

    cart_misc_func_t  mbc_reset_func;
    cart_misc_func_t  mbc_deinit_func;
    
    void *mbc_state;
} gb_cart_t;

/**
 * Initializes the instance of the cartridge
 * 
 * \param cart Cartridge instance
 * \param rom_path ROM file path
 */
gbstatus_e cart_init(gb_cart_t *cart, const char *rom_path);

/**
 * Resets the cartridge
 * 
 * \param cart Cartridge instance
 */
void cart_reset(gb_cart_t *cart);

/**
 * Emulates a memory read request to the cartridge
 * 
 * \param cart Cartridge instance
 * \param addr Address to read
 * \return Byte read
 */
uint8_t cart_read(gb_cart_t *cart, uint16_t addr);

/**
 * Emulates a memory write request to the cartridge
 * 
 * \param cart Cartridge instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void cart_write(gb_cart_t *cart, uint16_t addr, uint8_t byte);

/**
 * Deinitializes the instance of the cartridge
 * 
 * \param cart Cartridge instance
 */
void cart_deinit(gb_cart_t *cart);

#endif