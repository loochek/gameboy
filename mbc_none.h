#ifndef MBC_NONE_H
#define MBC_NONE_H

#include <stdint.h>
#include "gbstatus.h"

/**
 * Represents cartridge with no mapper logic
 */

struct gb_cart;

/**
 * Initializes zero mapper
 * 
 * \param cart Cartridge instance
 */
gbstatus_e mbc_none_init(struct gb_cart *cart);

/**
 * Resets zero mapper
 * 
 * \param cart Cartridge instance
 */
gbstatus_e mbc_none_reset(struct gb_cart *cart);

/**
 * Implements memory reading requests to the cartridge without mapper
 * 
 * \param cart Cartridge instance
 * \param addr Address to read
 * \param byte_out Where to store read byte
 */
gbstatus_e mbc_none_read(struct gb_cart *cart, uint16_t addr, uint8_t *byte_out);

/**
 * Implements memory writing requests to the cartridge without mapper
 * 
 * \param cart Cartridge instance
 * \param addr Address to write
 * \param byte Byte to write
 */
gbstatus_e mbc_none_write(struct gb_cart *cart, uint16_t addr, uint8_t byte);

/**
 * Deinitializes zero mapper
 * 
 * \param cart Cartridge instance
 */
gbstatus_e mbc_none_deinit(struct gb_cart *cart);

#endif