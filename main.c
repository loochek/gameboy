#include <stdio.h>
#include "gb.h"

gbstatus_e run()
{
    gbstatus_e status = GBSTATUS_OK;

    gb_t gb = {0};
    gb_cpu_t cpu = {0};
    gb_mmu_t mmu = {0};
    gb_int_controller_t intr_ctl = {0};
    
    gb.cpu       = &cpu;
    gb.mmu       = &mmu;
    gb.intr_ctrl = &intr_ctl;

    GBCHK(cpu_init(gb.cpu, &gb));
    GBCHK(mmu_init(gb.mmu, &gb, "tests/11-op a,(hl).gb"));
    GBCHK(int_init(gb.intr_ctrl, &gb));

    for (int i = 0; i < 1000000000; i++)
    {
        //GBCHK(cpu_dump(gb.cpu));
        GBCHK(cpu_step(gb.cpu));
    }

    GBCHK(mmu_deinit(gb.mmu));

    return GBSTATUS_OK;
}

int main()
{
    gbstatus_e status = run();

    if (status != GBSTATUS_OK)
    {
        GBSTATUS_ERR_PRINT();
        return -1;
    }

    return 0;
}