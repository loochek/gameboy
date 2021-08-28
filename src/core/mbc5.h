#ifndef MBC5_H
#define MBC5_H

#include <stdint.h>
#include "gbstatus.h"

/**
 * Represents cartridge with MBC5 mapper
 */

struct gb_cart;

/**
 * Initializes MBC5
 * 
 * \param cart Cartridge instance
 */
gbstatus_e mbc5_init(struct gb_cart *cart);

/**
 * Resets MBC5
 * 
 * \param cart Cartridge instance
 */
void mbc5_reset(struct gb_cart *cart);

/**
 * Implements memory reading requests to the cartridge with MBC5
 * 
 * \param cart Cartridge instance
 * \param addr Address to read
 * \return Byte read
 */
uint8_t mbc5_read(struct gb_cart *cart, uint16_t addr);

/**
 * Implements memory writing requests to the cartridge with MBC5
 * 
 * \param cart Cartridge instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void mbc5_write(struct gb_cart *cart, uint16_t addr, uint8_t byte);

/**
 * Deinitializes MBC5
 * 
 * \param cart Cartridge instance
 */
void mbc5_deinit(struct gb_cart *cart);

#endif