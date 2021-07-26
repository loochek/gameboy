#include "gb.h"

gbstatus_e sync_with_cpu(gb_t *gb, int elapsed_cycles)
{
    GBCHK(timer_update(gb->timer, elapsed_cycles));
    GBCHK(ppu_update(gb->ppu, elapsed_cycles));
    
    return GBSTATUS_OK;
}