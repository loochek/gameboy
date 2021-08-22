#ifndef MBC1_H
#define MBC1_H

#include <stdint.h>
#include "gbstatus.h"

/**
 * Represents cartridge with MBC1 mapper
 */

struct gb_cart;

/**
 * Initializes MBC1
 * 
 * \param cart Cartridge instance
 */
gbstatus_e mbc1_init(struct gb_cart *cart);

/**
 * Resets MBC1
 * 
 * \param cart Cartridge instance
 */
void mbc1_reset(struct gb_cart *cart);

/**
 * Implements memory reading requests to the cartridge with MBC1
 * 
 * \param cart Cartridge instance
 * \param addr Address to read
 * \return Byte read
 */
uint8_t mbc1_read(struct gb_cart *cart, uint16_t addr);

/**
 * Implements memory writing requests to the cartridge with MBC1
 * 
 * \param cart Cartridge instance
 * \param addr Address to write
 * \param byte Byte to write
 */
void mbc1_write(struct gb_cart *cart, uint16_t addr, uint8_t byte);

/**
 * Deinitializes MBC1
 * 
 * \param cart Cartridge instance
 */
void mbc1_deinit(struct gb_cart *cart);

#endif