#include "mmu.h"

gbstatus_e mmu_read(gb_mmu_t *mmu, uint16_t addr, uint8_t *byte_out)
{
    // NOP placeholder
    *byte_out = 0x00;

    return GBSTATUS_OK;
}

gbstatus_e mmu_write(gb_mmu_t *mmu, uint16_t addr, uint8_t byte)
{
    // do nothing
    return GBSTATUS_OK;
}