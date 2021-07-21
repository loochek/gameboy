#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdint.h>
#include "gbstatus.h"
#include "gb.h"

/**
 * LR35902 state description
 */
typedef struct gb_cpu
{
    /// TODO: deal with big-endian machines

    /// Registers

    union
    {
        struct
        {
            uint8_t reg_f;
            uint8_t reg_a;
        };

        uint16_t reg_af;
    };

    union
    {
        struct
        {
            uint8_t reg_c;
            uint8_t reg_b;
        };

        uint16_t reg_bc;
    };

    union
    {
        struct
        {
            uint8_t reg_e;
            uint8_t reg_d;
        };

        uint16_t reg_de;
    };

    union
    {
        struct
        {
            uint8_t reg_l;
            uint8_t reg_h;
        };

        uint16_t reg_hl;
    };

    /// Program counter
    uint16_t pc;

    /// Stack pointer
    uint16_t sp;

    /// Pointer to the Gameboy structure
    gb_t *gb;

} gb_cpu_t;

/**
 * Fetches and executes one CPU instruction
 * 
 * \param cpu CPU instance
 */
gbstatus_e cpu_step(gb_cpu_t *cpu);

#endif