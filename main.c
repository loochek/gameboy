#include <stdio.h>
#include "gb.h"
#include "cart.h"

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

    status = cpu_init(gb.cpu, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup0;

    status = mmu_init(gb.mmu, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup0;

    status = int_init(gb.intr_ctrl, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup1;

    status = timer_init(gb.timer, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup1;

    gb_cart_t cart = {0};
    status = cart_init(&cart, rom_path);
    if (status != GBSTATUS_OK)
        goto cleanup1;

    status = mmu_switch_cart(gb.mmu, &cart);
    if (status != GBSTATUS_OK)
        goto cleanup2;

    while (true)
    {
        status = cpu_step(gb.cpu);
        if (status != GBSTATUS_OK)
            goto cleanup2;
    }

cleanup2:
    GBCHK(cart_deinit(&cart));

cleanup1:
    GBCHK(mmu_deinit(gb.mmu));

cleanup0:
    return status;
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