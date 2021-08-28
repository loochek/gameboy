#ifndef MBC2_H
#define MBC2_H

#include <stdint.h>
#include "gbstatus.h"

/**
 * Represents cartridge with MBC2 mapper
 */

struct gb_cart;

/**
 * Initializes MBC2
 * 
 * \param cart Cartridge instance
 */
gbstatus_e mbc2_init(struct gb_cart *cart);

/**
 * Resets MBC2
 * 
 * \param cart Cartridge instance
 */
void mbc2_reset(struct gb_cart *cart);

/**
 * Implements memory reading requests to the cartridge with MBC2
 * 
 * \param cart Cartridge instance
 * \param addr Address to read
 * \return Byte read
 */
uint8_t mbc2_read(struct gb_cart *cart, uint16_t addr);

/**
 * Implements memory writing requests to the cartridge with MBC2
 * 
 * \param cart Cartridge instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void mbc2_write(struct gb_cart *cart, uint16_t addr, uint8_t byte);

/**
 * Deinitializes MBC2
 * 
 * \param cart Cartridge instance
 */
void mbc2_deinit(struct gb_cart *cart);

#endif