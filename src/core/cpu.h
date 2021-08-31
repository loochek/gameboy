#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "gbstatus.h"

struct gb;

/**
 * Gameboy CPU representation
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

    bool halted;

    int ei_delay;

    /// Interrupt master switch
    bool ime;

    /// Program counter
    uint16_t pc;

    /// Stack pointer
    uint16_t sp;

    /// Pointer to the parent Gameboy structure
    struct gb *gb;

} gb_cpu_t;

/**
 * Initializes the instance of the CPU
 * 
 * \param cpu CPU instance
 * \param gb Parent GB instance
 */
void cpu_init(gb_cpu_t *cpu, struct gb *gb);

/**
 * Resets the CPU
 * 
 * \param cpu CPU instance
 */
void cpu_reset(gb_cpu_t *cpu);

/**
 * Sets PC to 0x0100
 * 
 * \param cpu CPU instance
 */
void cpu_skip_bootrom(gb_cpu_t *cpu);

/**
 * Makes an interrupt request to the CPU
 * 
 * \param cpu CPU instance
 * \param int_vec Interrupt vector
 * \return False if interrupts are disabled, true otherwise
 */
bool cpu_irq(gb_cpu_t *cpu, uint16_t int_vec);

/**
 * Dumps CPU state to the standart output
 * 
 * \param cpu CPU instance
 */
void cpu_dump(gb_cpu_t *cpu);

/**
 * Fetches and executes one CPU instruction
 * 
 * \param cpu CPU instance
 */
gbstatus_e cpu_step(gb_cpu_t *cpu);

#endif
