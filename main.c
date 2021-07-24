#include <stdio.h>
#include "gb.h"

gbstatus_e run(const char *rom_path)
{
    gbstatus_e status = GBSTATUS_OK;

    gb_t gb = {0};
    gb_cpu_t cpu = {0};
    gb_mmu_t mmu = {0};
    gb_int_controller_t intr_ctl = {0};
    gb_timer_t timer = {0};
    
    gb.cpu       = &cpu;
    gb.mmu       = &mmu;
    gb.intr_ctrl = &intr_ctl;
    gb.timer     = &timer;

    GBCHK(cpu_init(gb.cpu, &gb));
    GBCHK(mmu_init(gb.mmu, &gb, rom_path));
    GBCHK(int_init(gb.intr_ctrl, &gb));
    GBCHK(timer_init(gb.timer, &gb));

    while (true)
    {
        //GBCHK(cpu_dump(gb.cpu));
        GBCHK(cpu_step(gb.cpu));
    }

    GBCHK(mmu_deinit(gb.mmu));

    return GBSTATUS_OK;
}

int main(int argc, const char *argv[])
{
    gbstatus_e status = run(argv[1]);

    if (status != GBSTATUS_OK)
    {
        GBSTATUS_ERR_PRINT();
        return -1;
    }

    return 0;
}