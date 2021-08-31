#include <assert.h>
#include "cpu.h"
#include "mmu.h"
#include "gb.h"

/// Memory access duration in clock cycles
#define MEM_ACCESS_DURATION 4

#define DISASM(instr, ...)
//#define DISASM(instr, ...) printf(instr "\n", ##__VA_ARGS__)

/// Sets 7th bit of the flags register
#define SET_Z(val) (cpu->reg_f = (cpu->reg_f & 0x7F) | ((val) << 7))

/// Sets 6th bit of the flags register
#define SET_N(val) (cpu->reg_f = (cpu->reg_f & 0xBF) | ((val) << 6))

/// Sets 5th bit of the flags register
#define SET_H(val) (cpu->reg_f = (cpu->reg_f & 0xDF) | ((val) << 5))

/// Sets 4th bit of the flags register
#define SET_C(val) (cpu->reg_f = (cpu->reg_f & 0xEF) | ((val) << 4))

/// Gets 7th bit of the flags register
#define GET_Z() ((cpu->reg_f >> 7) & 0x1)

/// Gets 6th bit of the flags register
#define GET_N() ((cpu->reg_f >> 6) & 0x1)

/// Gets 5th bit of the flags register
#define GET_H() ((cpu->reg_f >> 5) & 0x1)

/// Gets 4th bit of the flags register
#define GET_C() ((cpu->reg_f >> 4) & 0x1)

#define ZERO_CHECK(val) (((val) & 0xFF) == 0)

#define CHECK_CARRY_4(val)  (((val) >> 4)  != 0)
#define CHECK_CARRY_8(val)  (((val) >> 8)  != 0)
#define CHECK_CARRY_12(val) (((val) >> 12) != 0)
#define CHECK_CARRY_16(val) (((val) >> 16) != 0)

// Helper functions and common implementations of some instructions

static inline void     cpu_jump           (gb_cpu_t *cpu, uint16_t location);
static inline void     cpu_instr_add_hl   (gb_cpu_t *cpu, uint16_t value);
static inline void     cpu_instr_rst      (gb_cpu_t *cpu, uint16_t vector);
static inline void     cpu_instr_ret_cond (gb_cpu_t *cpu, bool condition);
static inline uint16_t cpu_instr_jp_cond  (gb_cpu_t *cpu, bool condition);
static inline uint16_t cpu_instr_call_cond(gb_cpu_t *cpu, bool condition);
static inline int8_t   cpu_instr_jr_cond  (gb_cpu_t *cpu, bool condition);

static inline void cpu_instr_add (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_adc (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_sub (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_cp  (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_sbc (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_and (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_or  (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_xor (gb_cpu_t *cpu, uint8_t value);
static inline void cpu_instr_inc (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_dec (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_rl  (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_rlc (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_rr  (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_rrc (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_sla (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_sra (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_srl (gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_swap(gb_cpu_t *cpu, uint8_t *value_ptr);
static inline void cpu_instr_bit (gb_cpu_t *cpu, int bit, uint8_t *value_ptr);
static inline void cpu_instr_res (gb_cpu_t *cpu, int bit, uint8_t *value_ptr);
static inline void cpu_instr_set (gb_cpu_t *cpu, int bit, uint8_t *value_ptr);


/**
 * Updates the state of peripherals according to elapsed clock cycles.
 */
static void sync_with_cpu(gb_cpu_t *cpu, int elapsed_cycles);

/**
 * Emulates a memory read request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to read
 * \return Byte read
 */
static uint8_t cpu_mem_read(gb_cpu_t *cpu, uint16_t addr);

/**
 * Emulates a memory read request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to read
 * \return Word read
 */
static uint16_t cpu_mem_read_word(gb_cpu_t *cpu, uint16_t addr);

/**
 * Emulates a memory write request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
static void cpu_mem_write(gb_cpu_t *cpu, uint16_t addr, uint8_t byte);

/**
 * Emulates a memory write request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to write
 * \param byte Word to write
 */
static void cpu_mem_write_word(gb_cpu_t *cpu, uint16_t addr, uint16_t word);

/**
 * CB-prefixed opcodes handler
 * 
 * \param cpu CPU instance
 */
static void cpu_step_cb(gb_cpu_t *cpu);


void cpu_init(gb_cpu_t *cpu, gb_t *gb)
{
    assert(cpu != NULL);
    assert(gb  != NULL);

    cpu->gb = gb;
    cpu_reset(cpu);
}

void cpu_reset(gb_cpu_t *cpu)
{
    assert(cpu != NULL);

    cpu->reg_af = 0x01B0;
    cpu->reg_bc = 0x0013;
    cpu->reg_de = 0x00D8;
    cpu->reg_hl = 0x014D;
    cpu->sp     = 0xFFFE;
    cpu->pc     = 0x0000;

    cpu->ime      = false;
    cpu->halted   = false;
    cpu->ei_delay = 0;
}

void cpu_skip_bootrom(gb_cpu_t *cpu)
{
    assert(cpu != NULL);
    
    cpu->pc = 0x0100;
}

bool cpu_irq(gb_cpu_t *cpu, uint16_t int_vec)
{
    assert(cpu != NULL);

    // note than unhalting is performed even if interrupts are disabled
    if (cpu->halted)
    {
        cpu->halted = false;
        cpu->pc++;

        sync_with_cpu(cpu, 4); // exiting halt mode
    }

    if (!cpu->ime)
        return false;

    cpu->ime = false;

    sync_with_cpu(cpu, 8);

    cpu->sp -= 2;
    cpu_mem_write_word(cpu, cpu->sp, cpu->pc);

    sync_with_cpu(cpu, 4);
    cpu->pc = int_vec;

    return true;
}

void cpu_dump(gb_cpu_t *cpu)
{
    assert(cpu != NULL);

    printf("======== CPU state ========\n");
    printf("| PC: 0x%04x   SP: 0x%04x |\n", cpu->pc, cpu->sp);
    printf("| AF: 0x%04x   BC: 0x%04x |\n", cpu->reg_af, cpu->reg_bc);
    printf("| DE: 0x%04x   HL: 0x%04x |\n", cpu->reg_de, cpu->reg_hl);
    printf("| Z: %d N: %d H: %d C: %d     |\n", GET_Z(), GET_N(), GET_H(), GET_C());
    printf("| IME: %d EI delay: %d      |\n", cpu->ime, cpu->ei_delay);
    printf("===========================\n");
}

gbstatus_e cpu_step(gb_cpu_t *cpu)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(cpu != NULL);

    if (cpu->ei_delay != 0)
    {
        cpu->ei_delay--;
        if (cpu->ei_delay == 0)
            cpu->ime = true;
    }

    uint8_t opcode = cpu_mem_read(cpu, cpu->pc);
    cpu->pc++;

    uint8_t  imm_val8  = 0;
    uint16_t imm_val16 = 0;

    switch (opcode)
    {
    // 8-bit loads
#pragma region
    // ld r8, r8
#pragma region 
    case 0x40:
        cpu->reg_b = cpu->reg_b;

        DISASM("ld b, b");
        break;

    case 0x41:
        cpu->reg_b = cpu->reg_c;

        DISASM("ld b, c");
        break;

    case 0x42:
        cpu->reg_b = cpu->reg_d;

        DISASM("ld b, d");
        break;

    case 0x43:
        cpu->reg_b = cpu->reg_e;

        DISASM("ld b, e");
        break;

    case 0x44:
        cpu->reg_b = cpu->reg_h;

        DISASM("ld b, h");
        break;
        
    case 0x45:
        cpu->reg_b = cpu->reg_l;

        DISASM("ld b, l");
        break;

    case 0x47:
        cpu->reg_b = cpu->reg_a;

        DISASM("ld b, a");
        break;

    case 0x48:
        cpu->reg_c = cpu->reg_b;

        DISASM("ld c, b");
        break;

    case 0x49:
        cpu->reg_c = cpu->reg_c;

        DISASM("ld c, c");
        break;

    case 0x4A:
        cpu->reg_c = cpu->reg_d;

        DISASM("ld c, d");
        break;

    case 0x4B:
        cpu->reg_c = cpu->reg_e;

        DISASM("ld c, e");
        break;

    case 0x4C:
        cpu->reg_c = cpu->reg_h;

        DISASM("ld c, h");
        break;

    case 0x4D:
        cpu->reg_c = cpu->reg_l;

        DISASM("ld c, l");
        break;

    case 0x4F:
        cpu->reg_c = cpu->reg_a;

        DISASM("ld c, a");
        break;

    case 0x50:
        cpu->reg_d = cpu->reg_b;

        DISASM("ld d, b");
        break;

    case 0x51:
        cpu->reg_d = cpu->reg_c;

        DISASM("ld d, c");
        break;

    case 0x52:
        cpu->reg_d = cpu->reg_d;

        DISASM("ld d, d");
        break;

    case 0x53:
        cpu->reg_d = cpu->reg_e;

        DISASM("ld d, e");
        break;

    case 0x54:
        cpu->reg_d = cpu->reg_h;

        DISASM("ld d, h");
        break;
        
    case 0x55:
        cpu->reg_d = cpu->reg_l;

        DISASM("ld d, l");
        break;

    case 0x57:
        cpu->reg_d = cpu->reg_a;

        DISASM("ld d, a");
        break;

    case 0x58:
        cpu->reg_e = cpu->reg_b;

        DISASM("ld e, b");
        break;

    case 0x59:
        cpu->reg_e = cpu->reg_c;

        DISASM("ld e, c");
        break;

    case 0x5A:
        cpu->reg_e = cpu->reg_d;

        DISASM("ld e, d");
        break;

    case 0x5B:
        cpu->reg_e = cpu->reg_e;

        DISASM("ld e, e");
        break;

    case 0x5C:
        cpu->reg_e = cpu->reg_h;

        DISASM("ld e, h");
        break;

    case 0x5D:
        cpu->reg_e = cpu->reg_l;

        DISASM("ld e, l");
        break;

    case 0x5F:
        cpu->reg_e = cpu->reg_a;

        DISASM("ld e, a");
        break;

    case 0x60:
        cpu->reg_h = cpu->reg_b;

        DISASM("ld h, b");
        break;

    case 0x61:
        cpu->reg_h = cpu->reg_c;

        DISASM("ld h, c");
        break;

    case 0x62:
        cpu->reg_h = cpu->reg_d;

        DISASM("ld h, d");
        break;

    case 0x63:
        cpu->reg_h = cpu->reg_e;

        DISASM("ld h, e");
        break;

    case 0x64:
        cpu->reg_h = cpu->reg_h;

        DISASM("ld h, h");
        break;
        
    case 0x65:
        cpu->reg_h = cpu->reg_l;

        DISASM("ld h, l");
        break;

    case 0x67:
        cpu->reg_h = cpu->reg_a;

        DISASM("ld h, a");
        break;

    case 0x68:
        cpu->reg_l = cpu->reg_b;

        DISASM("ld l, b");
        break;

    case 0x69:
        cpu->reg_l = cpu->reg_c;

        DISASM("ld l, c");
        break;

    case 0x6A:
        cpu->reg_l = cpu->reg_d;

        DISASM("ld l, d");
        break;

    case 0x6B:
        cpu->reg_l = cpu->reg_e;

        DISASM("ld l, e");
        break;

    case 0x6C:
        cpu->reg_l = cpu->reg_h;

        DISASM("ld l, h");
        break;

    case 0x6D:
        cpu->reg_l = cpu->reg_l;

        DISASM("ld l, l");
        break;

    case 0x6F:
        cpu->reg_l = cpu->reg_a;

        DISASM("ld l, a");
        break;

    case 0x78:
        cpu->reg_a = cpu->reg_b;

        DISASM("ld a, b");
        break;

    case 0x79:
        cpu->reg_a = cpu->reg_c;

        DISASM("ld a, c");
        break;

    case 0x7A:
        cpu->reg_a = cpu->reg_d;

        DISASM("ld a, d");
        break;

    case 0x7B:
        cpu->reg_a = cpu->reg_e;

        DISASM("ld a, e");
        break;

    case 0x7C:
        cpu->reg_a = cpu->reg_h;

        DISASM("ld a, h");
        break;

    case 0x7D:
        cpu->reg_a = cpu->reg_l;

        DISASM("ld a, l");
        break;

    case 0x7F:
        cpu->reg_a = cpu->reg_a;

        DISASM("ld a, a");
        break;
#pragma endregion

    // ld (hl), r8
#pragma region
    case 0x70:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_b);

        DISASM("ld (hl), b");
        break;

    case 0x71:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_c);

        DISASM("ld (hl), c");
        break;

    case 0x72:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_d);

        DISASM("ld (hl), d");
        break;

    case 0x73:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_e);

        DISASM("ld (hl), e");
        break;

    case 0x74:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_h);

        DISASM("ld (hl), h");
        break;

    case 0x75:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_l);

        DISASM("ld (hl), l");
        break;

    case 0x77:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_a);

        DISASM("ld (hl), a");
        break;
#pragma endregion

    // ld r8, (hl)
#pragma region
    case 0x46:
        cpu->reg_b = cpu_mem_read(cpu, cpu->reg_hl);

        DISASM("ld b, (hl)");
        break;

    case 0x4E:
        cpu->reg_c = cpu_mem_read(cpu, cpu->reg_hl);

        DISASM("ld c, (hl)");
        break;

    case 0x56:
        cpu->reg_d = cpu_mem_read(cpu, cpu->reg_hl);

        DISASM("ld d, (hl)");
        break;

    case 0x5E:
        cpu->reg_e = cpu_mem_read(cpu, cpu->reg_hl);
        DISASM("ld e, (hl)");
        break;

    case 0x66:
        cpu->reg_h = cpu_mem_read(cpu, cpu->reg_hl);

        DISASM("ld h, (hl)");
        break;

    case 0x6E:
        cpu->reg_l = cpu_mem_read(cpu, cpu->reg_hl);

        DISASM("ld l, (hl)");
        break;

    case 0x7E:
        cpu->reg_a = cpu_mem_read(cpu, cpu->reg_hl);

        DISASM("ld a, (hl)");
        break;

#pragma endregion

    // ld r8, imm8
#pragma region
    case 0x06:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;
        cpu->reg_b = imm_val8;

        DISASM("ld b, 0x%02x", imm_val8);
        break;

    case 0x0E:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;
        cpu->reg_c = imm_val8;

        DISASM("ld c, 0x%02x", imm_val8);
        break;

    case 0x16:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;
        cpu->reg_d = imm_val8;

        DISASM("ld d, 0x%02x", imm_val8);
        break;

    case 0x1E:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;
        cpu->reg_e = imm_val8;

        DISASM("ld e, 0x%02x", imm_val8);
        break;

    case 0x26:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;
        cpu->reg_h = imm_val8;

        DISASM("ld h, 0x%02x", imm_val8);
        break;

    case 0x2E:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;
        cpu->reg_l = imm_val8;

        DISASM("ld l, 0x%02x", imm_val8);
        break;

    case 0x3E:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;
        cpu->reg_a = imm_val8;

        DISASM("ld a, 0x%02x", imm_val8);
        break;
#pragma endregion

    // etc 8-bit loads
#pragma region
    case 0x02:
        cpu_mem_write(cpu, cpu->reg_bc, cpu->reg_a);

        DISASM("ld (bc), a");
        break;

    case 0x12:
        cpu_mem_write(cpu, cpu->reg_de, cpu->reg_a);
        
        DISASM("ld (de), a");
        break;

    case 0x22:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_a);
        cpu->reg_hl++;
        
        DISASM("ld (hl++), a");
        break;

    case 0x32:
        cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_a);
        cpu->reg_hl--;
        
        DISASM("ld (hl--), a");
        break;

    case 0x0A:
        cpu->reg_a = cpu_mem_read(cpu, cpu->reg_bc);

        DISASM("ld a, (bc)");
        break;

    case 0x1A:
        cpu->reg_a = cpu_mem_read(cpu, cpu->reg_de);

        DISASM("ld a, (de)");
        break;

    case 0x2A:
        cpu->reg_a = cpu_mem_read(cpu, cpu->reg_hl);
        cpu->reg_hl++;

        DISASM("ld a, (hl++)");
        break;

    case 0x3A:
        cpu->reg_a = cpu_mem_read(cpu, cpu->reg_hl);
        cpu->reg_hl--;

        DISASM("ld a, (hl--)");
        break;

    case 0x36:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("ld (hl), 0x%02x", imm_val8);
        break;

    case 0xE0:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_mem_write(cpu, 0xFF00 + imm_val8, cpu->reg_a);
        
        DISASM("ld (0xFF00 + 0x%02x), a", imm_val8);
        break;

    case 0xF0:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu->reg_a = cpu_mem_read(cpu, 0xFF00 + imm_val8);

        DISASM("ld a, (0xFF00 + 0x%02x)", imm_val8);
        break;

    case 0xE2:
        cpu_mem_write(cpu, 0xFF00 + cpu->reg_c, cpu->reg_a);
        
        DISASM("ld (0xFF00 + c), a");
        break;

    case 0xF2:
        cpu->reg_a = cpu_mem_read(cpu, 0xFF00 + cpu->reg_c);

        DISASM("ld a, (0xFF00 + c)");
        break;

    case 0xEA:
        imm_val16 = cpu_mem_read_word(cpu, cpu->pc);
        cpu->pc += 2;

        cpu_mem_write(cpu, imm_val16, cpu->reg_a);

        DISASM("ld (0x%04x), a", imm_val16);
        break;

    case 0xFA:
        imm_val16 = cpu_mem_read_word(cpu, cpu->pc);
        cpu->pc += 2;

        cpu->reg_a = cpu_mem_read(cpu, imm_val16);

        DISASM("ld a, (0x%04x)", imm_val16);
        break;
#pragma endregion
#pragma endregion

    // 16-bit loads
#pragma region
    // ld r16, imm16
#pragma region
    case 0x01:
        imm_val16 = cpu_mem_read_word(cpu, cpu->pc);
        cpu->pc += 2;

        cpu->reg_bc = imm_val16;

        DISASM("ld bc, 0x%04x", imm_val16);
        break;

    case 0x11:
        imm_val16 = cpu_mem_read_word(cpu, cpu->pc);
        cpu->pc += 2;

        cpu->reg_de = imm_val16;

        DISASM("ld de, 0x%04x", imm_val16);
        break;

    case 0x21:
        imm_val16 = cpu_mem_read_word(cpu, cpu->pc);
        cpu->pc += 2;

        cpu->reg_hl = imm_val16;

        DISASM("ld hl, 0x%04x", imm_val16);
        break;

    case 0x31:
        imm_val16 = cpu_mem_read_word(cpu, cpu->pc);
        cpu->pc += 2;

        cpu->sp = imm_val16;

        DISASM("ld sp, 0x%04x", imm_val16);
        break;
#pragma endregion

    // pop r16
#pragma region
    case 0xC1:
        imm_val16 = cpu_mem_read_word(cpu, cpu->sp);
        cpu->sp += 2;

        cpu->reg_bc = imm_val16;

        DISASM("pop bc");
        break;

    case 0xD1:
        imm_val16 = cpu_mem_read_word(cpu, cpu->sp);
        cpu->sp += 2;

        cpu->reg_de = imm_val16;

        DISASM("pop de");
        break;

    case 0xE1:
        imm_val16 = cpu_mem_read_word(cpu, cpu->sp);
        cpu->sp += 2;

        cpu->reg_hl = imm_val16;

        DISASM("pop hl");
        break;

    case 0xF1:
        imm_val16 = cpu_mem_read_word(cpu, cpu->sp);
        cpu->sp += 2;

        cpu->reg_af = imm_val16;
        cpu->reg_f &= 0xf0; // least 4 bits of the flags register must be always zero

        DISASM("pop af");
        break;
#pragma endregion

    // push r16
#pragma region
    case 0xC5:
        sync_with_cpu(cpu, 4); // internal timing

        cpu->sp -= 2;
        cpu_mem_write_word(cpu, cpu->sp, cpu->reg_bc);

        DISASM("push bc");
        break;

    case 0xD5:
        sync_with_cpu(cpu, 4); // internal timing

        cpu->sp -= 2;
        cpu_mem_write_word(cpu, cpu->sp, cpu->reg_de);

        DISASM("push de");
        break;

    case 0xE5:
        sync_with_cpu(cpu, 4); // internal timing
        
        cpu->sp -= 2;
        cpu_mem_write_word(cpu, cpu->sp, cpu->reg_hl);

        DISASM("push hl");
        break;

    case 0xF5:
        sync_with_cpu(cpu, 4); // internal timing
        
        cpu->sp -= 2;
        cpu_mem_write_word(cpu, cpu->sp, cpu->reg_af);

        DISASM("push af");
        break;
#pragma endregion

    // etc 16-bit loads
#pragma region
    case 0x08:
        imm_val16 = cpu_mem_read_word(cpu, cpu->pc);
        cpu->pc += 2;

        cpu_mem_write_word(cpu, imm_val16, cpu->sp);
        DISASM("ld (0x%04x), sp", imm_val16);
        break;

    case 0xF9:
        cpu->sp = cpu->reg_hl;
        sync_with_cpu(cpu, 4); // internal timing

        DISASM("ld sp, hl");
        break;
#pragma endregion
#pragma endregion
    
    // 8-bit arithmetics
#pragma region
    // add a, r8
#pragma region
    case 0x80:
        cpu_instr_add(cpu, cpu->reg_b);

        DISASM("add a, b");
        break;

    case 0x81:
        cpu_instr_add(cpu, cpu->reg_c);
        
        DISASM("add a, c");
        break;

    case 0x82:
        cpu_instr_add(cpu, cpu->reg_d);
        
        DISASM("add a, d");
        break;

    case 0x83:
        cpu_instr_add(cpu, cpu->reg_e);
        
        DISASM("add a, e");
        break;

    case 0x84:
        cpu_instr_add(cpu, cpu->reg_h);
        
        DISASM("add a, h");
        break;

    case 0x85:
        cpu_instr_add(cpu, cpu->reg_l);
        
        DISASM("add a, l");
        break;

    case 0x87:
        cpu_instr_add(cpu, cpu->reg_a);
        
        DISASM("add a, a");
        break;
#pragma endregion

    // adc a, r8
#pragma region
    case 0x88:
        cpu_instr_adc(cpu, cpu->reg_b);

        DISASM("adc a, b");
        break;

    case 0x89:
        cpu_instr_adc(cpu, cpu->reg_c);
        
        DISASM("adc a, c");
        break;

    case 0x8A:
        cpu_instr_adc(cpu, cpu->reg_d);
        
        DISASM("adc a, d");
        break;

    case 0x8B:
        cpu_instr_adc(cpu, cpu->reg_e);
        
        DISASM("adc a, e");
        break;

    case 0x8C:
        cpu_instr_adc(cpu, cpu->reg_h);
        
        DISASM("adc a, h");
        break;

    case 0x8D:
        cpu_instr_adc(cpu, cpu->reg_l);
        
        DISASM("adc a, l");
        break;

    case 0x8F:
        cpu_instr_adc(cpu, cpu->reg_a);
        
        DISASM("adc a, a");
        break;
#pragma endregion

    // sub a, r8
#pragma region
    case 0x90:
        cpu_instr_sub(cpu, cpu->reg_b);

        DISASM("sub a, b");
        break;

    case 0x91:
        cpu_instr_sub(cpu, cpu->reg_c);
        
        DISASM("sub a, c");
        break;

    case 0x92:
        cpu_instr_sub(cpu, cpu->reg_d);
        
        DISASM("sub a, d");
        break;

    case 0x93:
        cpu_instr_sub(cpu, cpu->reg_e);
        
        DISASM("sub a, e");
        break;

    case 0x94:
        cpu_instr_sub(cpu, cpu->reg_h);
        
        DISASM("sub a, h");
        break;

    case 0x95:
        cpu_instr_sub(cpu, cpu->reg_l);
        
        DISASM("sub a, l");
        break;

    case 0x97:
        cpu_instr_sub(cpu, cpu->reg_a);
        
        DISASM("sub a, a");
        break;
#pragma endregion

    // sbc a, r8
#pragma region
    case 0x98:
        cpu_instr_sbc(cpu, cpu->reg_b);

        DISASM("sbc a, b");
        break;

    case 0x99:
        cpu_instr_sbc(cpu, cpu->reg_c);
        
        DISASM("sbc a, c");
        break;

    case 0x9A:
        cpu_instr_sbc(cpu, cpu->reg_d);
        
        DISASM("sbc a, d");
        break;

    case 0x9B:
        cpu_instr_sbc(cpu, cpu->reg_e);
        
        DISASM("sbc a, e");
        break;

    case 0x9C:
        cpu_instr_sbc(cpu, cpu->reg_h);
        
        DISASM("sbc a, h");
        break;

    case 0x9D:
        cpu_instr_sbc(cpu, cpu->reg_l);
        
        DISASM("sbc a, l");
        break;

    case 0x9F:
        cpu_instr_sbc(cpu, cpu->reg_a);
        
        DISASM("sbc a, a");
        break;
#pragma endregion

     // and a, r8
#pragma region
    case 0xA0:
        cpu_instr_and(cpu, cpu->reg_b);

        DISASM("and a, b");
        break;

    case 0xA1:
        cpu_instr_and(cpu, cpu->reg_c);
        
        DISASM("and a, c");
        break;

    case 0xA2:
        cpu_instr_and(cpu, cpu->reg_d);
        
        DISASM("and a, d");
        break;

    case 0xA3:
        cpu_instr_and(cpu, cpu->reg_e);
        
        DISASM("and a, e");
        break;

    case 0xA4:
        cpu_instr_and(cpu, cpu->reg_h);
        
        DISASM("and a, h");
        break;

    case 0xA5:
        cpu_instr_and(cpu, cpu->reg_l);
        
        DISASM("and a, l");
        break;

    case 0xA7:
        cpu_instr_and(cpu, cpu->reg_a);
        
        DISASM("and a, a");
        break;
#pragma endregion

    // xor a, r8
#pragma region
    case 0xA8:
        cpu_instr_xor(cpu, cpu->reg_b);

        DISASM("xor a, b");
        break;

    case 0xA9:
        cpu_instr_xor(cpu, cpu->reg_c);
        
        DISASM("xor a, c");
        break;

    case 0xAA:
        cpu_instr_xor(cpu, cpu->reg_d);
        
        DISASM("xor a, d");
        break;

    case 0xAB:
        cpu_instr_xor(cpu, cpu->reg_e);
        
        DISASM("xor a, e");
        break;

    case 0xAC:
        cpu_instr_xor(cpu, cpu->reg_h);
        
        DISASM("xor a, h");
        break;

    case 0xAD:
        cpu_instr_xor(cpu, cpu->reg_l);
        
        DISASM("xor a, l");
        break;

    case 0xAF:
        cpu_instr_xor(cpu, cpu->reg_a);
        
        DISASM("xor a, a");
        break;
#pragma endregion

    // or a, r8
#pragma region
    case 0xB0:
        cpu_instr_or(cpu, cpu->reg_b);

        DISASM("or a, b");
        break;

    case 0xB1:
        cpu_instr_or(cpu, cpu->reg_c);
        
        DISASM("or a, c");
        break;

    case 0xB2:
        cpu_instr_or(cpu, cpu->reg_d);
        
        DISASM("or a, d");
        break;

    case 0xB3:
        cpu_instr_or(cpu, cpu->reg_e);
        
        DISASM("or a, e");
        break;

    case 0xB4:
        cpu_instr_or(cpu, cpu->reg_h);
        
        DISASM("or a, h");
        break;

    case 0xB5:
        cpu_instr_or(cpu, cpu->reg_l);
        
        DISASM("or a, l");
        break;

    case 0xB7:
        cpu_instr_or(cpu, cpu->reg_a);
        
        DISASM("or a, a");
        break;
#pragma endregion

    // cp a, r8
#pragma region
    case 0xB8:
        cpu_instr_cp(cpu, cpu->reg_b);

        DISASM("cp a, b");
        break;

    case 0xB9:
        cpu_instr_cp(cpu, cpu->reg_c);
        
        DISASM("cp a, c");
        break;

    case 0xBA:
        cpu_instr_cp(cpu, cpu->reg_d);
        
        DISASM("cp a, d");
        break;

    case 0xBB:
        cpu_instr_cp(cpu, cpu->reg_e);
        
        DISASM("cp a, e");
        break;

    case 0xBC:
        cpu_instr_cp(cpu, cpu->reg_h);
        
        DISASM("cp a, h");
        break;

    case 0xBD:
        cpu_instr_cp(cpu, cpu->reg_l);
        
        DISASM("cp a, l");
        break;

    case 0xBF:
        cpu_instr_cp(cpu, cpu->reg_a);
        
        DISASM("cp a, a");
        break;
#pragma endregion

    // inc r8
#pragma region
    case 0x04:
        cpu_instr_inc(cpu, &cpu->reg_b);

        DISASM("inc b");
        break;
    
    case 0x0c:
        cpu_instr_inc(cpu, &cpu->reg_c);
        
        DISASM("inc c");
        break;

    case 0x14:
        cpu_instr_inc(cpu, &cpu->reg_d);

        DISASM("inc d");
        break;
    
    case 0x1c:
        cpu_instr_inc(cpu, &cpu->reg_e);
        
        DISASM("inc e");
        break;

    case 0x24:
        cpu_instr_inc(cpu, &cpu->reg_h);

        DISASM("inc h");
        break;
    
    case 0x2c:
        cpu_instr_inc(cpu, &cpu->reg_l);
        
        DISASM("inc l");
        break;

    case 0x3c:
        cpu_instr_inc(cpu, &cpu->reg_a);

        DISASM("inc a");
        break;
#pragma endregion
    
    // dec r8
#pragma region
    case 0x05:
        cpu_instr_dec(cpu, &cpu->reg_b);

        DISASM("dec b");
        break;
    
    case 0x0d:
        cpu_instr_dec(cpu, &cpu->reg_c);
        
        DISASM("dec c");
        break;

    case 0x15:
        cpu_instr_dec(cpu, &cpu->reg_d);

        DISASM("dec d");
        break;
    
    case 0x1d:
        cpu_instr_dec(cpu, &cpu->reg_e);
        
        DISASM("dec e");
        break;

    case 0x25:
        cpu_instr_dec(cpu, &cpu->reg_h);

        DISASM("dec h");
        break;
    
    case 0x2d:
        cpu_instr_dec(cpu, &cpu->reg_l);
        
        DISASM("dec l");
        break;

    case 0x3d:
        cpu_instr_dec(cpu, &cpu->reg_a);

        DISASM("dec a");
        break;
#pragma endregion

    // OP a, (hl)
#pragma region
    case 0x86:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_add(cpu, imm_val8);

        DISASM("add a, (hl)");
        break;

    case 0x96:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_sub(cpu, imm_val8);

        DISASM("sub a, (hl)");
        break;

    case 0xA6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_and(cpu, imm_val8);

        DISASM("and a, (hl)");
        break;

    case 0xB6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_or(cpu, imm_val8);

        DISASM("or a, (hl)");
        break;

    case 0x8E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_adc(cpu, imm_val8);

        DISASM("adc a, (hl)");
        break;

    case 0x9E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_sbc(cpu, imm_val8);

        DISASM("sbc a, (hl)");
        break;

    case 0xAE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_xor(cpu, imm_val8);

        DISASM("xor a, (hl)");
        break;

    case 0xBE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_cp(cpu, imm_val8);

        DISASM("cp a, (hl)");
        break;
#pragma endregion

    // OP a, imm8
#pragma region
    case 0xC6:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_add(cpu, imm_val8);

        DISASM("add a, 0x%02x", imm_val8);
        break;

    case 0xD6:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_sub(cpu, imm_val8);

        DISASM("sub a, 0x%02x", imm_val8);
        break;

    case 0xE6:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_and(cpu, imm_val8);

        DISASM("and a, 0x%02x", imm_val8);
        break;

    case 0xF6:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_or(cpu, imm_val8);

        DISASM("or a, 0x%02x", imm_val8);
        break;

    case 0xCE:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_adc(cpu, imm_val8);

        DISASM("adc a, 0x%02x", imm_val8);
        break;

    case 0xDE:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_sbc(cpu, imm_val8);

        DISASM("sbc a, 0x%02x", imm_val8);
        break;

    case 0xEE:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_xor(cpu, imm_val8);

        DISASM("xor a, 0x%02x", imm_val8);
        break;

    case 0xFE:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        cpu_instr_cp(cpu, imm_val8);

        DISASM("cp a, 0x%02x", imm_val8);
        break;
#pragma endregion

    // misc 8-bit arithmetic
#pragma region
    case 0x34:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_inc(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("inc (hl)");
        break;

    case 0x35:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_dec(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);
        
        DISASM("dec (hl)");
        break;

    case 0x37:
        SET_N(0);
        SET_H(0);
        SET_C(1);

        DISASM("scf");
        break;

    case 0x3F:
        SET_N(0);
        SET_H(0);
        SET_C(1 - GET_C());

        DISASM("ccf");
        break;

    case 0x2F:
        SET_N(1);
        SET_H(1);

        cpu->reg_a = ~cpu->reg_a;

        DISASM("cpl");
        break;


    case 0x27:
    {
        int val = cpu->reg_a;
        int correction = 0;
        bool carry = false;

        if (GET_H() || (!GET_N() && ((val & 0xf) > 0x9)))
            correction += 0x6;
        
        if (GET_C() || (!GET_N() && val > 0x99))
        {
            correction += 0x60;
            carry = true;
        }

        if (GET_N())
            val -= correction;
        else
            val += correction;

        SET_Z(ZERO_CHECK(val));
        SET_H(0);
        SET_C(carry);

        cpu->reg_a = val;

        DISASM("DAA");
        break;
    }   
#pragma endregion
#pragma endregion

    // 16-bit arithmetics
#pragma region
    // inc r16
#pragma region
    case 0x03:
        cpu->reg_bc++;
        sync_with_cpu(cpu, 4); // internal

        DISASM("inc bc");
        break;

    case 0x13:
        cpu->reg_de++;
        sync_with_cpu(cpu, 4); // internal
        
        DISASM("inc de");
        break;

    case 0x23:
        cpu->reg_hl++;
        sync_with_cpu(cpu, 4); // internal
        
        DISASM("inc hl");
        break;

    case 0x33:
        cpu->sp++;
        sync_with_cpu(cpu, 4); // internal
        
        DISASM("inc sp");
        break;
#pragma endregion

    // dec r16
#pragma region
    case 0x0B:
        cpu->reg_bc--;
        sync_with_cpu(cpu, 4); // internal

        DISASM("dec bc");
        break;

    case 0x1B:
        cpu->reg_de--;
        sync_with_cpu(cpu, 4); // internal
        
        DISASM("dec de");
        break;

    case 0x2B:
        cpu->reg_hl--;
        sync_with_cpu(cpu, 4); // internal
        
        DISASM("dec hl");
        break;

    case 0x3B:
        cpu->sp--;
        sync_with_cpu(cpu, 4); // internal
        
        DISASM("dec sp");
        break;
#pragma endregion

    // add hl, r16
#pragma region
    case 0x09:
        cpu_instr_add_hl(cpu, cpu->reg_bc);

        DISASM("add hl, bc");
        break;

    case 0x19:
        cpu_instr_add_hl(cpu, cpu->reg_de);

        DISASM("add hl, de");
        break;

    case 0x29:
        cpu_instr_add_hl(cpu, cpu->reg_hl);

        DISASM("add hl, hl");
        break;

    case 0x39:
        cpu_instr_add_hl(cpu, cpu->sp);

        DISASM("add hl, sp");
        break;
#pragma endregion

    // misc 16-bit arithmetics
#pragma region
    case 0xE8:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        SET_Z(0);
        SET_N(0);
        SET_H(CHECK_CARRY_4((cpu->sp & 0xF) + (imm_val8 & 0xF)));
        SET_C(CHECK_CARRY_8((cpu->sp & 0xFF) + imm_val8));

        cpu->sp += (int8_t)imm_val8;

        sync_with_cpu(cpu, 8); // internal

        DISASM("add sp, %d", (int8_t)imm_val8);
        break;

    case 0xF8:
        imm_val8 = cpu_mem_read(cpu, cpu->pc);
        cpu->pc++;

        SET_Z(0);
        SET_N(0);
        SET_H(CHECK_CARRY_4((cpu->sp & 0xF) + (imm_val8 & 0xF)));
        SET_C(CHECK_CARRY_8((cpu->sp & 0xFF) + imm_val8));

        cpu->reg_hl = cpu->sp + (int8_t)imm_val8;

        sync_with_cpu(cpu, 4); // internal

        DISASM("ld hl, sp%+d", (int8_t)imm_val8);
        break;
#pragma endregion
#pragma endregion
    
    // control (branches)
#pragma region
    // absolute jumps
#pragma region
    case 0xC2:
        imm_val16 = cpu_instr_jp_cond(cpu, 1 - GET_Z());

        DISASM("jp nz, 0x%04x", imm_val16);
        break;

    case 0xD2:
        imm_val16 = cpu_instr_jp_cond(cpu, 1 - GET_C());

        DISASM("jp nc, 0x%04x", imm_val16);
        break;

    case 0xCA:
        imm_val16 = cpu_instr_jp_cond(cpu, GET_Z());

        DISASM("jp z, 0x%04x", imm_val16);
        break;

    case 0xDA:
        imm_val16 = cpu_instr_jp_cond(cpu, GET_C());

        DISASM("jp c, 0x%04x", imm_val16);
        break;

    case 0xC3:
        imm_val16 = cpu_instr_jp_cond(cpu, true);

        DISASM("jp 0x%04x", imm_val16);
        break;

    case 0xE9:
        cpu->pc = cpu->reg_hl;

        DISASM("jp hl");
        break;
#pragma endregion

    // relative jumps
#pragma region
    case 0x20:
        imm_val8 = cpu_instr_jr_cond(cpu, 1 - GET_Z());

        DISASM("jr nz, %d", imm_val8);
        break;

    case 0x30:
        imm_val8 = cpu_instr_jr_cond(cpu, 1 - GET_C());

        DISASM("jr nc, %d", imm_val8);
        break;

    case 0x28:
        imm_val8 = cpu_instr_jr_cond(cpu, GET_Z());

        DISASM("jr z, %d", imm_val8);
        break;

    case 0x38:
        imm_val8 = cpu_instr_jr_cond(cpu, GET_C());

        DISASM("jr c, %d", imm_val8);
        break;

    case 0x18:
        imm_val8 = cpu_instr_jr_cond(cpu, true);

        DISASM("jr %d", imm_val8);
        break;
#pragma endregion

    // calls
#pragma region
    case 0xC4:
        imm_val16 = cpu_instr_call_cond(cpu, 1 - GET_Z());

        DISASM("call nz, 0x%04x", imm_val16);
        break;

    case 0xD4:
        imm_val16 = cpu_instr_call_cond(cpu, 1 - GET_C());

        DISASM("call nc, 0x%04x", imm_val16);
        break;

    case 0xCC:
        imm_val16 = cpu_instr_call_cond(cpu, GET_Z());

        DISASM("call z, 0x%04x", imm_val16);
        break;

    case 0xDC:
        imm_val16 = cpu_instr_call_cond(cpu, GET_C());

        DISASM("call c, 0x%04x", imm_val16);
        break;

    case 0xCD:
        imm_val16 = cpu_instr_call_cond(cpu, true);

        DISASM("call 0x%04x", imm_val16);
        break;
#pragma endregion

    // returns
#pragma region
    case 0xC0:
        cpu_instr_ret_cond(cpu, 1 - GET_Z());

        DISASM("ret nz");
        break;

    case 0xD0:
        cpu_instr_ret_cond(cpu, 1 - GET_C());

        DISASM("ret nc");
        break;

    case 0xC8:
        cpu_instr_ret_cond(cpu, GET_Z());

        DISASM("ret z");
        break;

    case 0xD8:
        cpu_instr_ret_cond(cpu, GET_C());

        DISASM("ret c");
        break;

    case 0xC9:
        imm_val16 = cpu_mem_read_word(cpu, cpu->sp);
        cpu->sp += 2;

        cpu_jump(cpu, imm_val16);

        DISASM("ret");
        break;

    case 0xD9:
        imm_val16 = cpu_mem_read_word(cpu, cpu->sp);
        cpu->sp += 2;

        cpu_jump(cpu, imm_val16);

        cpu->ime = true;
        DISASM("reti");
        break;
#pragma endregion

    // resets
#pragma region
    case 0xC7:
        cpu_instr_rst(cpu, 0x00);

        DISASM("rst 00h");
        break;

    case 0xD7:
        cpu_instr_rst(cpu, 0x10);

        DISASM("rst 10h");
        break;

    case 0xE7:
        cpu_instr_rst(cpu, 0x20);

        DISASM("rst 20h");
        break;

    case 0xF7:
        cpu_instr_rst(cpu, 0x30);

        DISASM("rst 30h");
        break;

    case 0xCF:
        cpu_instr_rst(cpu, 0x08);

        DISASM("rst 08h");
        break;

    case 0xDF:
        cpu_instr_rst(cpu, 0x18);

        DISASM("rst 18h");
        break;

    case 0xEF:
        cpu_instr_rst(cpu, 0x28);

        DISASM("rst 28h");
        break;

    case 0xFF:
        cpu_instr_rst(cpu, 0x38);

        DISASM("rst 38h");
        break;
#pragma endregion
#pragma endregion

    // control (misc)
#pragma region
    case 0x00:
        DISASM("nop");
        break;

    case 0x10:
        /// TODO: stop
        DISASM("stop");
        break;

    case 0x76:
        cpu->halted = true;
        cpu->pc--;

        DISASM("halt");
        break;

    case 0xF3:
        cpu->ime = false;
        cpu->ei_delay = 0;

        DISASM("di");
        break;

    case 0xFB:
        cpu->ime = false;
        cpu->ei_delay = 2;

        DISASM("ei");
        break;
#pragma endregion

    // misc
#pragma region
    case 0x07:
        cpu_instr_rlc(cpu, &cpu->reg_a);
        SET_Z(0);

        DISASM("rlca");
        break;

    case 0x17:
        cpu_instr_rl(cpu, &cpu->reg_a);
        SET_Z(0);

        DISASM("rla");
        break;

    case 0x0F:
        cpu_instr_rrc(cpu, &cpu->reg_a);
        SET_Z(0);

        DISASM("rrca");
        break;

    case 0x1F:
        cpu_instr_rr(cpu, &cpu->reg_a);
        SET_Z(0);

        DISASM("rra");
        break;

    case 0xCB:
        cpu_step_cb(cpu);
        break;

    case 0xD3:
    case 0xE3:
    case 0xE4:
    case 0xF4:
    case 0xDB:
    case 0xEB:
    case 0xEC:
    case 0xFC:
    case 0xDD:
    case 0xED:
    case 0xFD:
        GBSTATUS(GBSTATUS_CPU_ILLEGAL_OP, "illegal opcode: 0x%02x", opcode);
        return status;

#pragma endregion
    }

    int_step(&cpu->gb->intr_ctrl);
    return GBSTATUS_OK;
}


static void sync_with_cpu(gb_cpu_t *cpu, int elapsed_cycles)
{
    gb_t *gb = cpu->gb;

    timer_update(&gb->timer, elapsed_cycles);
    ppu_update(&gb->ppu, elapsed_cycles);
}

static uint8_t cpu_mem_read(gb_cpu_t *cpu, uint16_t addr)
{
    // Memory access takes some time
    sync_with_cpu(cpu, MEM_ACCESS_DURATION);
    return mmu_read(&cpu->gb->mmu, addr);
}

static uint16_t cpu_mem_read_word(gb_cpu_t *cpu, uint16_t addr)
{
    uint16_t word = 0;

    // Memory access takes some time
    sync_with_cpu(cpu, MEM_ACCESS_DURATION);
    word = mmu_read(&cpu->gb->mmu, addr);

    sync_with_cpu(cpu, MEM_ACCESS_DURATION);
    word |= mmu_read(&cpu->gb->mmu, addr + 1) << 8;

    return word;
}

static void cpu_mem_write(gb_cpu_t *cpu, uint16_t addr, uint8_t byte)
{
    // Memory access takes some time
    sync_with_cpu(cpu, MEM_ACCESS_DURATION);
    mmu_write(&cpu->gb->mmu, addr, byte);
}

static void cpu_mem_write_word(gb_cpu_t *cpu, uint16_t addr, uint16_t word)
{
    // Memory access takes some time
    sync_with_cpu(cpu, MEM_ACCESS_DURATION);
    mmu_write(&cpu->gb->mmu, addr, word & 0xFF);

    sync_with_cpu(cpu, MEM_ACCESS_DURATION);
    mmu_write(&cpu->gb->mmu, addr + 1, word >> 8);
}

static inline void cpu_jump(gb_cpu_t *cpu, uint16_t location)
{
    sync_with_cpu(cpu, 4);
    cpu->pc = location;
}

static inline void cpu_instr_add(gb_cpu_t *cpu, uint8_t value)
{
    SET_Z(ZERO_CHECK(cpu->reg_a + value));
    SET_N(0);
    SET_H(CHECK_CARRY_4((cpu->reg_a & 0xF) + (value & 0xF)));
    SET_C(CHECK_CARRY_8(cpu->reg_a + value));

    cpu->reg_a += value;
}

static inline void cpu_instr_adc(gb_cpu_t *cpu, uint8_t value)
{
    int carry = GET_C();

    SET_Z(ZERO_CHECK(cpu->reg_a + value + carry));
    SET_N(0);
    SET_H(CHECK_CARRY_4((cpu->reg_a & 0xF) + (value & 0xF) + carry));
    SET_C(CHECK_CARRY_8(cpu->reg_a + value + carry));

    cpu->reg_a += value + carry;
}

static inline void cpu_instr_sub(gb_cpu_t *cpu, uint8_t value)
{
    SET_Z(cpu->reg_a == value);
    SET_N(1);
    SET_H((cpu->reg_a & 0xF) < (value & 0xF));
    SET_C(cpu->reg_a < value);

    cpu->reg_a -= value;
}

static inline void cpu_instr_cp(gb_cpu_t *cpu, uint8_t value)
{
    SET_Z(cpu->reg_a == value);
    SET_N(1);
    SET_H((cpu->reg_a & 0xF) < (value & 0xF));
    SET_C(cpu->reg_a < value);
}

static inline void cpu_instr_sbc(gb_cpu_t *cpu, uint8_t value)
{
    int carry = GET_C();
    int result = cpu->reg_a - value - carry;

    SET_Z(ZERO_CHECK(result));
    SET_N(1);
    SET_H((cpu->reg_a & 0xF) - (value & 0xF) - carry < 0);
    SET_C(result < 0);

    cpu->reg_a = result;
}

static inline void cpu_instr_and(gb_cpu_t *cpu, uint8_t value)
{
    cpu->reg_a &= value;

    SET_Z(cpu->reg_a == 0);
    SET_N(0);
    SET_H(1);
    SET_C(0);
}

static inline void cpu_instr_or(gb_cpu_t *cpu, uint8_t value)
{
    cpu->reg_a |= value;

    SET_Z(cpu->reg_a == 0);
    SET_N(0);
    SET_H(0);
    SET_C(0);
}

static inline void cpu_instr_xor(gb_cpu_t *cpu, uint8_t value)
{
    cpu->reg_a ^= value;

    SET_Z(cpu->reg_a == 0);
    SET_N(0);
    SET_H(0);
    SET_C(0);
}

static inline void cpu_instr_inc(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    SET_Z(ZERO_CHECK(*value_ptr + 1));
    SET_N(0);
    SET_H(((*value_ptr) & 0xF) == 0xF);

    (*value_ptr)++;
}

static inline void cpu_instr_dec(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    SET_Z(ZERO_CHECK(*value_ptr - 1));
    SET_N(1);
    SET_H(((*value_ptr) & 0xF) == 0);

    (*value_ptr)--;
}

static inline void cpu_instr_add_hl(gb_cpu_t *cpu, uint16_t value)
{
    sync_with_cpu(cpu, 4); // internal

    SET_N(0);
    SET_H(CHECK_CARRY_12((cpu->reg_hl & 0xFFF) + (value & 0xFFF)));
    SET_C(CHECK_CARRY_16(cpu->reg_hl + value));

    cpu->reg_hl += value;
}

static inline uint16_t cpu_instr_jp_cond(gb_cpu_t *cpu, bool condition)
{
    uint16_t new_pc = cpu_mem_read_word(cpu, cpu->pc);
    cpu->pc += 2;

    if (condition)
        cpu_jump(cpu, new_pc);

    return new_pc;
}

static inline int8_t cpu_instr_jr_cond(gb_cpu_t *cpu, bool condition)
{
    int8_t disp = (int8_t)cpu_mem_read(cpu, cpu->pc);
    cpu->pc++;

    if (condition)
        cpu_jump(cpu, cpu->pc + disp);

    return disp;
}

static inline uint16_t cpu_instr_call_cond(gb_cpu_t *cpu, bool condition)
{
    uint16_t new_pc = cpu_mem_read_word(cpu, cpu->pc);
    cpu->pc += 2;

    if (condition)
    {
        cpu->sp -= 2;
        cpu_mem_write_word(cpu, cpu->sp, cpu->pc);
        cpu_jump(cpu, new_pc);
    }

    return new_pc;
}

static inline void cpu_instr_ret_cond(gb_cpu_t *cpu, bool condition)
{
    sync_with_cpu(cpu, 4); // internal

    if (condition)
    {
        uint16_t ret_addr = cpu_mem_read_word(cpu, cpu->sp);
        cpu->sp += 2;

        cpu_jump(cpu, ret_addr);
    }
}

static inline void cpu_instr_rst(gb_cpu_t *cpu, uint16_t vector)
{
    cpu->sp -= 2;
    cpu_mem_write_word(cpu, cpu->sp, cpu->pc);
    cpu_jump(cpu, vector);
}

static inline void cpu_instr_rl(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    int msb = *value_ptr >> 7;

    *value_ptr = (*value_ptr << 1) | GET_C();

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(msb);
}

static inline void cpu_instr_rlc(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    int msb = *value_ptr >> 7;

    *value_ptr = (*value_ptr << 1) | msb;

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(msb);
}

static inline void cpu_instr_rr(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    int lsb = *value_ptr & 0x1;

    *value_ptr = (*value_ptr >> 1) | (GET_C() << 7);

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(lsb);
}

static inline void cpu_instr_rrc(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    int lsb = *value_ptr & 0x1;

    *value_ptr = (*value_ptr >> 1) | (lsb << 7);

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(lsb);
}

static inline void cpu_instr_sla(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    int msb = *value_ptr >> 7;

    *value_ptr <<= 1;

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(msb);
}

static inline void cpu_instr_sra(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    int lsb = *value_ptr & 0x1;

    *value_ptr = (*value_ptr >> 1) | (*value_ptr & 0x80);

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(lsb);
}

static inline void cpu_instr_srl(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    int lsb = *value_ptr & 0x1;

    *value_ptr >>= 1;

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(lsb);
}

static inline void cpu_instr_swap(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    *value_ptr = (*value_ptr >> 4) | (*value_ptr << 4);

    SET_Z(ZERO_CHECK(*value_ptr));
    SET_N(0);
    SET_H(0);
    SET_C(0);
}

static inline void cpu_instr_bit(gb_cpu_t *cpu, int bit, uint8_t *value_ptr)
{
    SET_Z(1 - ((*value_ptr >> bit) & 0x1));
    SET_N(0);
    SET_H(1);
}

static inline void cpu_instr_res(gb_cpu_t *cpu, int bit, uint8_t *value_ptr)
{
    *value_ptr &= ~(1 << bit);
}

static inline void cpu_instr_set(gb_cpu_t *cpu, int bit, uint8_t *value_ptr)
{
    *value_ptr |= 1 << bit;
}

static void cpu_step_cb(gb_cpu_t *cpu)
{
    uint8_t opcode = cpu_mem_read(cpu, cpu->pc);
    cpu->pc++;

    uint8_t imm_val8 = 0;

    switch (opcode)
    {
    // rlc
#pragma region
    case 0x00:
        cpu_instr_rlc(cpu, &cpu->reg_b);

        DISASM("rlc b");
        break;

    case 0x01:
        cpu_instr_rlc(cpu, &cpu->reg_c);

        DISASM("rlc c");
        break;

    case 0x02:
        cpu_instr_rlc(cpu, &cpu->reg_d);

        DISASM("rlc d");
        break;

    case 0x03:
        cpu_instr_rlc(cpu, &cpu->reg_e);

        DISASM("rlc e");
        break;

    case 0x04:
        cpu_instr_rlc(cpu, &cpu->reg_h);

        DISASM("rlc h");
        break;

    case 0x05:
        cpu_instr_rlc(cpu, &cpu->reg_l);

        DISASM("rlc l");
        break;

    case 0x06:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_rlc(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("rlc (hl)");
        break;

    case 0x07:
        cpu_instr_rlc(cpu, &cpu->reg_a);

        DISASM("rlc a");
        break;
#pragma endregion

    // rrc
#pragma region
    case 0x08:
        cpu_instr_rrc(cpu, &cpu->reg_b);

        DISASM("rrc b");
        break;

    case 0x09:
        cpu_instr_rrc(cpu, &cpu->reg_c);

        DISASM("rrc c");
        break;

    case 0x0A:
        cpu_instr_rrc(cpu, &cpu->reg_d);

        DISASM("rrc d");
        break;

    case 0x0B:
        cpu_instr_rrc(cpu, &cpu->reg_e);

        DISASM("rrc e");
        break;

    case 0x0C:
        cpu_instr_rrc(cpu, &cpu->reg_h);

        DISASM("rrc h");
        break;

    case 0x0D:
        cpu_instr_rrc(cpu, &cpu->reg_l);

        DISASM("rrc l");
        break;

    case 0x0E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_rrc(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("rrc (hl)");
        break;

    case 0x0F:
        cpu_instr_rrc(cpu, &cpu->reg_a);

        DISASM("rrc a");
        break;
#pragma endregion
    
    // rl
#pragma region
    case 0x10:
        cpu_instr_rl(cpu, &cpu->reg_b);

        DISASM("rl b");
        break;

    case 0x11:
        cpu_instr_rl(cpu, &cpu->reg_c);

        DISASM("rl c");
        break;

    case 0x12:
        cpu_instr_rl(cpu, &cpu->reg_d);

        DISASM("rl d");
        break;

    case 0x13:
        cpu_instr_rl(cpu, &cpu->reg_e);

        DISASM("rl e");
        break;

    case 0x14:
        cpu_instr_rl(cpu, &cpu->reg_h);

        DISASM("rl h");
        break;

    case 0x15:
        cpu_instr_rl(cpu, &cpu->reg_l);

        DISASM("rl l");
        break;

    case 0x16:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_rl(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("rl (hl)");
        break;

    case 0x17:
        cpu_instr_rl(cpu, &cpu->reg_a);

        DISASM("rl a");
        break;
#pragma endregion

    // rr
#pragma region
    case 0x18:
        cpu_instr_rr(cpu, &cpu->reg_b);

        DISASM("rr b");
        break;

    case 0x19:
        cpu_instr_rr(cpu, &cpu->reg_c);

        DISASM("rr c");
        break;

    case 0x1A:
        cpu_instr_rr(cpu, &cpu->reg_d);

        DISASM("rr d");
        break;

    case 0x1B:
        cpu_instr_rr(cpu, &cpu->reg_e);

        DISASM("rr e");
        break;

    case 0x1C:
        cpu_instr_rr(cpu, &cpu->reg_h);

        DISASM("rr h");
        break;

    case 0x1D:
        cpu_instr_rr(cpu, &cpu->reg_l);

        DISASM("rr l");
        break;

    case 0x1E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_rr(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("rr (hl)");
        break;

    case 0x1F:
        cpu_instr_rr(cpu, &cpu->reg_a);

        DISASM("rr a");
        break;
#pragma endregion
    
    // sla
#pragma region
    case 0x20:
        cpu_instr_sla(cpu, &cpu->reg_b);

        DISASM("sla b");
        break;

    case 0x21:
        cpu_instr_sla(cpu, &cpu->reg_c);

        DISASM("sla c");
        break;

    case 0x22:
        cpu_instr_sla(cpu, &cpu->reg_d);

        DISASM("sla d");
        break;

    case 0x23:
        cpu_instr_sla(cpu, &cpu->reg_e);

        DISASM("sla e");
        break;

    case 0x24:
        cpu_instr_sla(cpu, &cpu->reg_h);

        DISASM("sla h");
        break;

    case 0x25:
        cpu_instr_sla(cpu, &cpu->reg_l);

        DISASM("sla l");
        break;

    case 0x26:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_sla(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("sla (hl)");
        break;

    case 0x27:
        cpu_instr_sla(cpu, &cpu->reg_a);

        DISASM("sla a");
        break;
#pragma endregion

    // sra
#pragma region
    case 0x28:
        cpu_instr_sra(cpu, &cpu->reg_b);

        DISASM("sra b");
        break;

    case 0x29:
        cpu_instr_sra(cpu, &cpu->reg_c);

        DISASM("sra c");
        break;

    case 0x2A:
        cpu_instr_sra(cpu, &cpu->reg_d);

        DISASM("sra d");
        break;

    case 0x2B:
        cpu_instr_sra(cpu, &cpu->reg_e);

        DISASM("sra e");
        break;

    case 0x2C:
        cpu_instr_sra(cpu, &cpu->reg_h);

        DISASM("sra h");
        break;

    case 0x2D:
        cpu_instr_sra(cpu, &cpu->reg_l);

        DISASM("sra l");
        break;

    case 0x2E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_sra(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("sra (hl)");
        break;

    case 0x2F:
        cpu_instr_sra(cpu, &cpu->reg_a);

        DISASM("sra a");
        break;
#pragma endregion
    
    // swap
#pragma region
    case 0x30:
        cpu_instr_swap(cpu, &cpu->reg_b);

        DISASM("swap b");
        break;

    case 0x31:
        cpu_instr_swap(cpu, &cpu->reg_c);

        DISASM("swap c");
        break;

    case 0x32:
        cpu_instr_swap(cpu, &cpu->reg_d);

        DISASM("swap d");
        break;

    case 0x33:
        cpu_instr_swap(cpu, &cpu->reg_e);

        DISASM("swap e");
        break;

    case 0x34:
        cpu_instr_swap(cpu, &cpu->reg_h);

        DISASM("swap h");
        break;

    case 0x35:
        cpu_instr_swap(cpu, &cpu->reg_l);

        DISASM("swap l");
        break;

    case 0x36:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_swap(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("swap (hl)");
        break;

    case 0x37:
        cpu_instr_swap(cpu, &cpu->reg_a);

        DISASM("swap a");
        break;
#pragma endregion

    // srl
#pragma region
    case 0x38:
        cpu_instr_srl(cpu, &cpu->reg_b);

        DISASM("srl b");
        break;

    case 0x39:
        cpu_instr_srl(cpu, &cpu->reg_c);

        DISASM("srl c");
        break;

    case 0x3A:
        cpu_instr_srl(cpu, &cpu->reg_d);

        DISASM("srl d");
        break;

    case 0x3B:
        cpu_instr_srl(cpu, &cpu->reg_e);

        DISASM("srl e");
        break;

    case 0x3C:
        cpu_instr_srl(cpu, &cpu->reg_h);

        DISASM("srl h");
        break;

    case 0x3D:
        cpu_instr_srl(cpu, &cpu->reg_l);

        DISASM("srl l");
        break;

    case 0x3E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_srl(cpu, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("srl (hl)");
        break;

    case 0x3F:
        cpu_instr_srl(cpu, &cpu->reg_a);

        DISASM("srl a");
        break;
#pragma endregion
    
    // bit
#pragma region
    case 0x40:
        cpu_instr_bit(cpu, 0, &cpu->reg_b);

        DISASM("bit 0, b");
        break;

    case 0x41:
        cpu_instr_bit(cpu, 0, &cpu->reg_c);

        DISASM("bit 0, c");
        break;

    case 0x42:
        cpu_instr_bit(cpu, 0, &cpu->reg_d);

        DISASM("bit 0, d");
        break;

    case 0x43:
        cpu_instr_bit(cpu, 0, &cpu->reg_e);

        DISASM("bit 0, e");
        break;

    case 0x44:
        cpu_instr_bit(cpu, 0, &cpu->reg_h);

        DISASM("bit 0, h");
        break;

    case 0x45:
        cpu_instr_bit(cpu, 0, &cpu->reg_l);

        DISASM("bit 0, l");
        break;

    case 0x46:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 0, &imm_val8);

        DISASM("bit 0, (hl)");
        break;

    case 0x47:
        cpu_instr_bit(cpu, 0, &cpu->reg_a);

        DISASM("bit 0, a");
        break;

    case 0x48:
        cpu_instr_bit(cpu, 1, &cpu->reg_b);

        DISASM("bit 1, b");
        break;

    case 0x49:
        cpu_instr_bit(cpu, 1, &cpu->reg_c);

        DISASM("bit 1, c");
        break;

    case 0x4A:
        cpu_instr_bit(cpu, 1, &cpu->reg_d);

        DISASM("bit 1, d");
        break;

    case 0x4B:
        cpu_instr_bit(cpu, 1, &cpu->reg_e);

        DISASM("bit 1, e");
        break;

    case 0x4C:
        cpu_instr_bit(cpu, 1, &cpu->reg_h);

        DISASM("bit 1, h");
        break;

    case 0x4D:
        cpu_instr_bit(cpu, 1, &cpu->reg_l);

        DISASM("bit 1, l");
        break;

    case 0x4E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 1, &imm_val8);

        DISASM("bit 1, (hl)");
        break;

    case 0x4F:
        cpu_instr_bit(cpu, 1, &cpu->reg_a);

        DISASM("bit 1, a");
        break;

    case 0x50:
        cpu_instr_bit(cpu, 2, &cpu->reg_b);

        DISASM("bit 2, b");
        break;

    case 0x51:
        cpu_instr_bit(cpu, 2, &cpu->reg_c);

        DISASM("bit 2, c");
        break;

    case 0x52:
        cpu_instr_bit(cpu, 2, &cpu->reg_d);

        DISASM("bit 2, d");
        break;

    case 0x53:
        cpu_instr_bit(cpu, 2, &cpu->reg_e);

        DISASM("bit 2, e");
        break;

    case 0x54:
        cpu_instr_bit(cpu, 2, &cpu->reg_h);

        DISASM("bit 2, h");
        break;

    case 0x55:
        cpu_instr_bit(cpu, 2, &cpu->reg_l);

        DISASM("bit 2, l");
        break;

    case 0x56:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 2, &imm_val8);

        DISASM("bit 2, (hl)");
        break;

    case 0x57:
        cpu_instr_bit(cpu, 2, &cpu->reg_a);

        DISASM("bit 2, a");
        break;

    case 0x58:
        cpu_instr_bit(cpu, 3, &cpu->reg_b);

        DISASM("bit 3, b");
        break;

    case 0x59:
        cpu_instr_bit(cpu, 3, &cpu->reg_c);

        DISASM("bit 3, c");
        break;

    case 0x5A:
        cpu_instr_bit(cpu, 3, &cpu->reg_d);

        DISASM("bit 3, d");
        break;

    case 0x5B:
        cpu_instr_bit(cpu, 3, &cpu->reg_e);

        DISASM("bit 3, e");
        break;

    case 0x5C:
        cpu_instr_bit(cpu, 3, &cpu->reg_h);

        DISASM("bit 3, h");
        break;

    case 0x5D:
        cpu_instr_bit(cpu, 3, &cpu->reg_l);

        DISASM("bit 3, l");
        break;

    case 0x5E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 3, &imm_val8);

        DISASM("bit 3, (hl)");
        break;

    case 0x5F:
        cpu_instr_bit(cpu, 3, &cpu->reg_a);

        DISASM("bit 3, a");
        break;

    case 0x60:
        cpu_instr_bit(cpu, 4, &cpu->reg_b);

        DISASM("bit 4, b");
        break;

    case 0x61:
        cpu_instr_bit(cpu, 4, &cpu->reg_c);

        DISASM("bit 4, c");
        break;

    case 0x62:
        cpu_instr_bit(cpu, 4, &cpu->reg_d);

        DISASM("bit 4, d");
        break;

    case 0x63:
        cpu_instr_bit(cpu, 4, &cpu->reg_e);

        DISASM("bit 4, e");
        break;

    case 0x64:
        cpu_instr_bit(cpu, 4, &cpu->reg_h);

        DISASM("bit 4, h");
        break;

    case 0x65:
        cpu_instr_bit(cpu, 4, &cpu->reg_l);

        DISASM("bit 4, l");
        break;

    case 0x66:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 4, &imm_val8);

        DISASM("bit 4, (hl)");
        break;

    case 0x67:
        cpu_instr_bit(cpu, 4, &cpu->reg_a);

        DISASM("bit 4, a");
        break;

    case 0x68:
        cpu_instr_bit(cpu, 5, &cpu->reg_b);

        DISASM("bit 5, b");
        break;

    case 0x69:
        cpu_instr_bit(cpu, 5, &cpu->reg_c);

        DISASM("bit 5, c");
        break;

    case 0x6A:
        cpu_instr_bit(cpu, 5, &cpu->reg_d);

        DISASM("bit 5, d");
        break;

    case 0x6B:
        cpu_instr_bit(cpu, 5, &cpu->reg_e);

        DISASM("bit 5, e");
        break;

    case 0x6C:
        cpu_instr_bit(cpu, 5, &cpu->reg_h);

        DISASM("bit 5, h");
        break;

    case 0x6D:
        cpu_instr_bit(cpu, 5, &cpu->reg_l);

        DISASM("bit 5, l");
        break;

    case 0x6E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 5, &imm_val8);

        DISASM("bit 5, (hl)");
        break;

    case 0x6F:
        cpu_instr_bit(cpu, 5, &cpu->reg_a);

        DISASM("bit 5, a");
        break;

    case 0x70:
        cpu_instr_bit(cpu, 6, &cpu->reg_b);

        DISASM("bit 6, b");
        break;

    case 0x71:
        cpu_instr_bit(cpu, 6, &cpu->reg_c);

        DISASM("bit 6, c");
        break;

    case 0x72:
        cpu_instr_bit(cpu, 6, &cpu->reg_d);

        DISASM("bit 6, d");
        break;

    case 0x73:
        cpu_instr_bit(cpu, 6, &cpu->reg_e);

        DISASM("bit 6, e");
        break;

    case 0x74:
        cpu_instr_bit(cpu, 6, &cpu->reg_h);

        DISASM("bit 6, h");
        break;

    case 0x75:
        cpu_instr_bit(cpu, 6, &cpu->reg_l);

        DISASM("bit 6, l");
        break;

    case 0x76:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 6, &imm_val8);

        DISASM("bit 6, (hl)");
        break;

    case 0x77:
        cpu_instr_bit(cpu, 6, &cpu->reg_a);

        DISASM("bit 6, a");
        break;

    case 0x78:
        cpu_instr_bit(cpu, 7, &cpu->reg_b);

        DISASM("bit 7, b");
        break;

    case 0x79:
        cpu_instr_bit(cpu, 7, &cpu->reg_c);

        DISASM("bit 7, c");
        break;

    case 0x7A:
        cpu_instr_bit(cpu, 7, &cpu->reg_d);

        DISASM("bit 7, d");
        break;

    case 0x7B:
        cpu_instr_bit(cpu, 7, &cpu->reg_e);

        DISASM("bit 7, e");
        break;

    case 0x7C:
        cpu_instr_bit(cpu, 7, &cpu->reg_h);

        DISASM("bit 7, h");
        break;

    case 0x7D:
        cpu_instr_bit(cpu, 7, &cpu->reg_l);

        DISASM("bit 7, l");
        break;

    case 0x7E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_bit(cpu, 7, &imm_val8);

        DISASM("bit 7, (hl)");
        break;

    case 0x7F:
        cpu_instr_bit(cpu, 7, &cpu->reg_a);

        DISASM("bit 7, a");
        break;
#pragma endregion
    
    // res
#pragma region
    case 0x80:
        cpu_instr_res(cpu, 0, &cpu->reg_b);

        DISASM("res 0, b");
        break;

    case 0x81:
        cpu_instr_res(cpu, 0, &cpu->reg_c);

        DISASM("res 0, c");
        break;

    case 0x82:
        cpu_instr_res(cpu, 0, &cpu->reg_d);

        DISASM("res 0, d");
        break;

    case 0x83:
        cpu_instr_res(cpu, 0, &cpu->reg_e);

        DISASM("res 0, e");
        break;

    case 0x84:
        cpu_instr_res(cpu, 0, &cpu->reg_h);

        DISASM("res 0, h");
        break;

    case 0x85:
        cpu_instr_res(cpu, 0, &cpu->reg_l);

        DISASM("res 0, l");
        break;

    case 0x86:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 0, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 0, (hl)");
        break;

    case 0x87:
        cpu_instr_res(cpu, 0, &cpu->reg_a);

        DISASM("res 0, a");
        break;

    case 0x88:
        cpu_instr_res(cpu, 1, &cpu->reg_b);

        DISASM("res 1, b");
        break;

    case 0x89:
        cpu_instr_res(cpu, 1, &cpu->reg_c);

        DISASM("res 1, c");
        break;

    case 0x8A:
        cpu_instr_res(cpu, 1, &cpu->reg_d);

        DISASM("res 1, d");
        break;

    case 0x8B:
        cpu_instr_res(cpu, 1, &cpu->reg_e);

        DISASM("res 1, e");
        break;

    case 0x8C:
        cpu_instr_res(cpu, 1, &cpu->reg_h);

        DISASM("res 1, h");
        break;

    case 0x8D:
        cpu_instr_res(cpu, 1, &cpu->reg_l);

        DISASM("res 1, l");
        break;

    case 0x8E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 1, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 1, (hl)");
        break;

    case 0x8F:
        cpu_instr_res(cpu, 1, &cpu->reg_a);

        DISASM("res 1, a");
        break;

    case 0x90:
        cpu_instr_res(cpu, 2, &cpu->reg_b);

        DISASM("res 2, b");
        break;

    case 0x91:
        cpu_instr_res(cpu, 2, &cpu->reg_c);

        DISASM("res 2, c");
        break;

    case 0x92:
        cpu_instr_res(cpu, 2, &cpu->reg_d);

        DISASM("res 2, d");
        break;

    case 0x93:
        cpu_instr_res(cpu, 2, &cpu->reg_e);

        DISASM("res 2, e");
        break;

    case 0x94:
        cpu_instr_res(cpu, 2, &cpu->reg_h);

        DISASM("res 2, h");
        break;

    case 0x95:
        cpu_instr_res(cpu, 2, &cpu->reg_l);

        DISASM("res 2, l");
        break;

    case 0x96:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 2, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 2, (hl)");
        break;

    case 0x97:
        cpu_instr_res(cpu, 2, &cpu->reg_a);

        DISASM("res 2, a");
        break;

    case 0x98:
        cpu_instr_res(cpu, 3, &cpu->reg_b);

        DISASM("res 3, b");
        break;

    case 0x99:
        cpu_instr_res(cpu, 3, &cpu->reg_c);

        DISASM("res 3, c");
        break;

    case 0x9A:
        cpu_instr_res(cpu, 3, &cpu->reg_d);

        DISASM("res 3, d");
        break;

    case 0x9B:
        cpu_instr_res(cpu, 3, &cpu->reg_e);

        DISASM("res 3, e");
        break;

    case 0x9C:
        cpu_instr_res(cpu, 3, &cpu->reg_h);

        DISASM("res 3, h");
        break;

    case 0x9D:
        cpu_instr_res(cpu, 3, &cpu->reg_l);

        DISASM("res 3, l");
        break;

    case 0x9E:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 3, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 3, (hl)");
        break;

    case 0x9F:
        cpu_instr_res(cpu, 3, &cpu->reg_a);

        DISASM("res 3, a");
        break;

    case 0xA0:
        cpu_instr_res(cpu, 4, &cpu->reg_b);

        DISASM("res 4, b");
        break;

    case 0xA1:
        cpu_instr_res(cpu, 4, &cpu->reg_c);

        DISASM("res 4, c");
        break;

    case 0xA2:
        cpu_instr_res(cpu, 4, &cpu->reg_d);

        DISASM("res 4, d");
        break;

    case 0xA3:
        cpu_instr_res(cpu, 4, &cpu->reg_e);

        DISASM("res 4, e");
        break;

    case 0xA4:
        cpu_instr_res(cpu, 4, &cpu->reg_h);

        DISASM("res 4, h");
        break;

    case 0xA5:
        cpu_instr_res(cpu, 4, &cpu->reg_l);

        DISASM("res 4, l");
        break;

    case 0xA6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 4, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 4, (hl)");
        break;

    case 0xA7:
        cpu_instr_res(cpu, 4, &cpu->reg_a);

        DISASM("res 4, a");
        break;

    case 0xA8:
        cpu_instr_res(cpu, 5, &cpu->reg_b);

        DISASM("res 5, b");
        break;

    case 0xA9:
        cpu_instr_res(cpu, 5, &cpu->reg_c);

        DISASM("res 5, c");
        break;

    case 0xAA:
        cpu_instr_res(cpu, 5, &cpu->reg_d);

        DISASM("res 5, d");
        break;

    case 0xAB:
        cpu_instr_res(cpu, 5, &cpu->reg_e);

        DISASM("res 5, e");
        break;

    case 0xAC:
        cpu_instr_res(cpu, 5, &cpu->reg_h);

        DISASM("res 5, h");
        break;

    case 0xAD:
        cpu_instr_res(cpu, 5, &cpu->reg_l);

        DISASM("res 5, l");
        break;

    case 0xAE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 5, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 5, (hl)");
        break;

    case 0xAF:
        cpu_instr_res(cpu, 5, &cpu->reg_a);

        DISASM("res 5, a");
        break;

    case 0xB0:
        cpu_instr_res(cpu, 6, &cpu->reg_b);

        DISASM("res 6, b");
        break;

    case 0xB1:
        cpu_instr_res(cpu, 6, &cpu->reg_c);

        DISASM("res 6, c");
        break;

    case 0xB2:
        cpu_instr_res(cpu, 6, &cpu->reg_d);

        DISASM("res 6, d");
        break;

    case 0xB3:
        cpu_instr_res(cpu, 6, &cpu->reg_e);

        DISASM("res 6, e");
        break;

    case 0xB4:
        cpu_instr_res(cpu, 6, &cpu->reg_h);

        DISASM("res 6, h");
        break;

    case 0xB5:
        cpu_instr_res(cpu, 6, &cpu->reg_l);

        DISASM("res 6, l");
        break;

    case 0xB6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 6, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 6, (hl)");
        break;

    case 0xB7:
        cpu_instr_res(cpu, 6, &cpu->reg_a);

        DISASM("res 6, a");
        break;

    case 0xB8:
        cpu_instr_res(cpu, 7, &cpu->reg_b);

        DISASM("res 7, b");
        break;

    case 0xB9:
        cpu_instr_res(cpu, 7, &cpu->reg_c);

        DISASM("res 7, c");
        break;

    case 0xBA:
        cpu_instr_res(cpu, 7, &cpu->reg_d);

        DISASM("res 7, d");
        break;

    case 0xBB:
        cpu_instr_res(cpu, 7, &cpu->reg_e);

        DISASM("res 7, e");
        break;

    case 0xBC:
        cpu_instr_res(cpu, 7, &cpu->reg_h);

        DISASM("res 7, h");
        break;

    case 0xBD:
        cpu_instr_res(cpu, 7, &cpu->reg_l);

        DISASM("res 7, l");
        break;

    case 0xBE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_res(cpu, 7, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("res 7, (hl)");
        break;

    case 0xBF:
        cpu_instr_res(cpu, 7, &cpu->reg_a);

        DISASM("res 7, a");
        break;
#pragma endregion
    
    // set
#pragma region
    case 0xC0:
        cpu_instr_set(cpu, 0, &cpu->reg_b);

        DISASM("set 0, b");
        break;

    case 0xC1:
        cpu_instr_set(cpu, 0, &cpu->reg_c);

        DISASM("set 0, c");
        break;

    case 0xC2:
        cpu_instr_set(cpu, 0, &cpu->reg_d);

        DISASM("set 0, d");
        break;

    case 0xC3:
        cpu_instr_set(cpu, 0, &cpu->reg_e);

        DISASM("set 0, e");
        break;

    case 0xC4:
        cpu_instr_set(cpu, 0, &cpu->reg_h);

        DISASM("set 0, h");
        break;

    case 0xC5:
        cpu_instr_set(cpu, 0, &cpu->reg_l);

        DISASM("set 0, l");
        break;

    case 0xC6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 0, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 0, (hl)");
        break;

    case 0xC7:
        cpu_instr_set(cpu, 0, &cpu->reg_a);

        DISASM("set 0, a");
        break;

    case 0xC8:
        cpu_instr_set(cpu, 1, &cpu->reg_b);

        DISASM("set 1, b");
        break;

    case 0xC9:
        cpu_instr_set(cpu, 1, &cpu->reg_c);

        DISASM("set 1, c");
        break;

    case 0xCA:
        cpu_instr_set(cpu, 1, &cpu->reg_d);

        DISASM("set 1, d");
        break;

    case 0xCB:
        cpu_instr_set(cpu, 1, &cpu->reg_e);

        DISASM("set 1, e");
        break;

    case 0xCC:
        cpu_instr_set(cpu, 1, &cpu->reg_h);

        DISASM("set 1, h");
        break;

    case 0xCD:
        cpu_instr_set(cpu, 1, &cpu->reg_l);

        DISASM("set 1, l");
        break;

    case 0xCE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 1, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 1, (hl)");
        break;

    case 0xCF:
        cpu_instr_set(cpu, 1, &cpu->reg_a);

        DISASM("set 1, a");
        break;

    case 0xD0:
        cpu_instr_set(cpu, 2, &cpu->reg_b);

        DISASM("set 2, b");
        break;

    case 0xD1:
        cpu_instr_set(cpu, 2, &cpu->reg_c);

        DISASM("set 2, c");
        break;

    case 0xD2:
        cpu_instr_set(cpu, 2, &cpu->reg_d);

        DISASM("set 2, d");
        break;

    case 0xD3:
        cpu_instr_set(cpu, 2, &cpu->reg_e);

        DISASM("set 2, e");
        break;

    case 0xD4:
        cpu_instr_set(cpu, 2, &cpu->reg_h);

        DISASM("set 2, h");
        break;

    case 0xD5:
        cpu_instr_set(cpu, 2, &cpu->reg_l);

        DISASM("set 2, l");
        break;

    case 0xD6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 2, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 2, (hl)");
        break;

    case 0xD7:
        cpu_instr_set(cpu, 2, &cpu->reg_a);

        DISASM("set 2, a");
        break;

    case 0xD8:
        cpu_instr_set(cpu, 3, &cpu->reg_b);

        DISASM("set 3, b");
        break;

    case 0xD9:
        cpu_instr_set(cpu, 3, &cpu->reg_c);

        DISASM("set 3, c");
        break;

    case 0xDA:
        cpu_instr_set(cpu, 3, &cpu->reg_d);

        DISASM("set 3, d");
        break;

    case 0xDB:
        cpu_instr_set(cpu, 3, &cpu->reg_e);

        DISASM("set 3, e");
        break;

    case 0xDC:
        cpu_instr_set(cpu, 3, &cpu->reg_h);

        DISASM("set 3, h");
        break;

    case 0xDD:
        cpu_instr_set(cpu, 3, &cpu->reg_l);

        DISASM("set 3, l");
        break;

    case 0xDE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 3, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 3, (hl)");
        break;

    case 0xDF:
        cpu_instr_set(cpu, 3, &cpu->reg_a);

        DISASM("set 3, a");
        break;

    case 0xE0:
        cpu_instr_set(cpu, 4, &cpu->reg_b);

        DISASM("set 4, b");
        break;

    case 0xE1:
        cpu_instr_set(cpu, 4, &cpu->reg_c);

        DISASM("set 4, c");
        break;

    case 0xE2:
        cpu_instr_set(cpu, 4, &cpu->reg_d);

        DISASM("set 4, d");
        break;

    case 0xE3:
        cpu_instr_set(cpu, 4, &cpu->reg_e);

        DISASM("set 4, e");
        break;

    case 0xE4:
        cpu_instr_set(cpu, 4, &cpu->reg_h);

        DISASM("set 4, h");
        break;

    case 0xE5:
        cpu_instr_set(cpu, 4, &cpu->reg_l);

        DISASM("set 4, l");
        break;

    case 0xE6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 4, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 4, (hl)");
        break;

    case 0xE7:
        cpu_instr_set(cpu, 4, &cpu->reg_a);

        DISASM("set 4, a");
        break;

    case 0xE8:
        cpu_instr_set(cpu, 5, &cpu->reg_b);

        DISASM("set 5, b");
        break;

    case 0xE9:
        cpu_instr_set(cpu, 5, &cpu->reg_c);

        DISASM("set 5, c");
        break;

    case 0xEA:
        cpu_instr_set(cpu, 5, &cpu->reg_d);

        DISASM("set 5, d");
        break;

    case 0xEB:
        cpu_instr_set(cpu, 5, &cpu->reg_e);

        DISASM("set 5, e");
        break;

    case 0xEC:
        cpu_instr_set(cpu, 5, &cpu->reg_h);

        DISASM("set 5, h");
        break;

    case 0xED:
        cpu_instr_set(cpu, 5, &cpu->reg_l);

        DISASM("set 5, l");
        break;

    case 0xEE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 5, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 5, (hl)");
        break;

    case 0xEF:
        cpu_instr_set(cpu, 5, &cpu->reg_a);

        DISASM("set 5, a");
        break;

    case 0xF0:
        cpu_instr_set(cpu, 6, &cpu->reg_b);

        DISASM("set 6, b");
        break;

    case 0xF1:
        cpu_instr_set(cpu, 6, &cpu->reg_c);

        DISASM("set 6, c");
        break;

    case 0xF2:
        cpu_instr_set(cpu, 6, &cpu->reg_d);

        DISASM("set 6, d");
        break;

    case 0xF3:
        cpu_instr_set(cpu, 6, &cpu->reg_e);

        DISASM("set 6, e");
        break;

    case 0xF4:
        cpu_instr_set(cpu, 6, &cpu->reg_h);

        DISASM("set 6, h");
        break;

    case 0xF5:
        cpu_instr_set(cpu, 6, &cpu->reg_l);

        DISASM("set 6, l");
        break;

    case 0xF6:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 6, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 6, (hl)");
        break;

    case 0xF7:
        cpu_instr_set(cpu, 6, &cpu->reg_a);

        DISASM("set 6, a");
        break;

    case 0xF8:
        cpu_instr_set(cpu, 7, &cpu->reg_b);

        DISASM("set 7, b");
        break;

    case 0xF9:
        cpu_instr_set(cpu, 7, &cpu->reg_c);

        DISASM("set 7, c");
        break;

    case 0xFA:
        cpu_instr_set(cpu, 7, &cpu->reg_d);

        DISASM("set 7, d");
        break;

    case 0xFB:
        cpu_instr_set(cpu, 7, &cpu->reg_e);

        DISASM("set 7, e");
        break;

    case 0xFC:
        cpu_instr_set(cpu, 7, &cpu->reg_h);

        DISASM("set 7, h");
        break;

    case 0xFD:
        cpu_instr_set(cpu, 7, &cpu->reg_l);

        DISASM("set 7, l");
        break;

    case 0xFE:
        imm_val8 = cpu_mem_read(cpu, cpu->reg_hl);
        cpu_instr_set(cpu, 7, &imm_val8);
        cpu_mem_write(cpu, cpu->reg_hl, imm_val8);

        DISASM("set 7, (hl)");
        break;

    case 0xFF:
        cpu_instr_set(cpu, 7, &cpu->reg_a);

        DISASM("set 7, a");
        break;
#pragma endregion    
    }
}