#include "cpu.h"
#include "mmu.h"

/// Memory access duration in clock cycles
#define MEM_ACCESS_DURATION 4

#define DISASM(instr, ...)

/**
 * Emulates a memory read request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to read
 * \param byte_out Where to store read byte
 */
static inline gbstatus_e cpu_mem_read(gb_cpu_t *cpu, uint16_t addr, uint8_t *byte_out)
{
    // Memory access takes some time
    GBCHK(sync_with_cpu(cpu->gb, MEM_ACCESS_DURATION));
    GBCHK(mmu_read(cpu->gb->mmu, addr, byte_out));

    return GBSTATUS_OK;
}

/**
 * Emulates a memory read request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to read
 * \param byte_out Where to store read word
 */
static inline gbstatus_e cpu_mem_read_word(gb_cpu_t *cpu, uint16_t addr, uint16_t *word_out)
{
    uint8_t byte = 0;

    // Memory access takes some time
    GBCHK(sync_with_cpu(cpu->gb, MEM_ACCESS_DURATION));
    GBCHK(mmu_read(cpu->gb->mmu, addr, &byte));
    *word_out |= byte;

    GBCHK(sync_with_cpu(cpu->gb, MEM_ACCESS_DURATION));
    GBCHK(mmu_read(cpu->gb->mmu, addr + 1, &byte));
    *word_out |= (byte << 8);

    return GBSTATUS_OK;
}

/**
 * Emulates a memory write request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to write
 * \param byte Byte to write
 */
static inline gbstatus_e cpu_mem_write(gb_cpu_t *cpu, uint16_t addr, uint8_t byte)
{
    // Memory access takes some time
    GBCHK(sync_with_cpu(cpu->gb, MEM_ACCESS_DURATION));
    GBCHK(mmu_write(cpu->gb->mmu, addr, byte));

    return GBSTATUS_OK;
}

/**
 * Emulates a memory write request from the CPU with correct timing
 * 
 * \param cpu CPU instance
 * \param addr Address to write
 * \param byte Word to write
 */
static inline gbstatus_e cpu_mem_write_word(gb_cpu_t *cpu, uint16_t addr, uint16_t word)
{
    // Memory access takes some time
    GBCHK(sync_with_cpu(cpu->gb, MEM_ACCESS_DURATION));
    GBCHK(mmu_write(cpu->gb->mmu, addr, word & 0xFF));

    GBCHK(sync_with_cpu(cpu->gb, MEM_ACCESS_DURATION));
    GBCHK(mmu_write(cpu->gb->mmu, addr + 1, word >> 8));

    return GBSTATUS_OK;
}

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

#define ZERO_CHECK(val) ((val & 0xFF) == 0)

#define CHECK_CARRY_4(val)  (((val) >> 4)  != 0)
#define CHECK_CARRY_8(val)  (((val) >> 8)  != 0)
#define CHECK_CARRY_12(val) (((val) >> 12) != 0)
#define CHECK_CARRY_16(val) (((val) >> 16) != 0)

// There are common implementations of some instructions

static void cpu_instr_add(gb_cpu_t *cpu, uint8_t value)
{
    SET_Z(ZERO_CHECK(cpu->reg_a + value));
    SET_N(0);
    SET_H(CHECK_CARRY_4((cpu->reg_a & 0xF) + (value & 0xF)));
    SET_C(CHECK_CARRY_8(cpu->reg_a + value));

    cpu->reg_a += value;
}

static void cpu_instr_adc(gb_cpu_t *cpu, uint8_t value)
{
    int carry = GET_C();

    SET_Z(ZERO_CHECK(cpu->reg_a + value + carry));
    SET_N(0);
    SET_H(CHECK_CARRY_4((cpu->reg_a & 0xF) + (value & 0xF) + carry));
    SET_C(CHECK_CARRY_8(cpu->reg_a + value + carry));

    cpu->reg_a += value + carry;
}

static void cpu_instr_sub(gb_cpu_t *cpu, uint8_t value)
{
    SET_Z(cpu->reg_a == value);
    SET_N(1);
    SET_H((cpu->reg_a & 0xF) < (value & 0xF));
    SET_C(cpu->reg_a < value);

    cpu->reg_a -= value;
}

static void cpu_instr_cp(gb_cpu_t *cpu, uint8_t value)
{
    SET_Z(cpu->reg_a == value);
    SET_N(1);
    SET_H((cpu->reg_a & 0xF) < (value & 0xF));
    SET_C(cpu->reg_a < value);
}

static void cpu_instr_sbc(gb_cpu_t *cpu, uint8_t value)
{
    int carry = GET_C();
    int result = cpu->reg_a - value - carry;

    SET_Z(ZERO_CHECK(result));
    SET_N(1);
    SET_H((cpu->reg_a & 0xF) - (value & 0xF) - carry < 0);
    SET_C(result < 0);

    cpu->reg_a = result;
}

static void cpu_instr_and(gb_cpu_t *cpu, uint8_t value)
{
    cpu->reg_a &= value;

    SET_Z(cpu->reg_a == 0);
    SET_N(0);
    SET_H(1);
    SET_C(0);
}

static void cpu_instr_or(gb_cpu_t *cpu, uint8_t value)
{
    cpu->reg_a |= value;

    SET_Z(cpu->reg_a == 0);
    SET_N(0);
    SET_H(0);
    SET_C(0);
}

static void cpu_instr_xor(gb_cpu_t *cpu, uint8_t value)
{
    cpu->reg_a ^= value;

    SET_Z(cpu->reg_a == 0);
    SET_N(0);
    SET_H(0);
    SET_C(0);
}

static void cpu_instr_inc(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    SET_Z(ZERO_CHECK(*value_ptr + 1));
    SET_N(0);
    SET_H((*value_ptr) & 0xF == 0xF);

    (*value_ptr)++;
}

static void cpu_instr_dec(gb_cpu_t *cpu, uint8_t *value_ptr)
{
    SET_Z(ZERO_CHECK(*value_ptr - 1));
    SET_N(1);
    SET_H((*value_ptr) & 0xF == 0);

    (*value_ptr)--;
}

gbstatus_e cpu_step(gb_cpu_t *cpu)
{
    gbstatus_e status = GBSTATUS_OK;

    if (cpu == NULL)
    {
        GBSTATUS(GBSTATUS_NULL_POINTER, "null pointer passed as CPU instance");
        return status;
    }

    uint8_t opcode = 0x00;
    GBCHK(cpu_mem_read(cpu, cpu->pc, &opcode));
    cpu->pc++;

    uint8_t  imm_val8  = 0;
    uint16_t imm_val16 = 0;

    switch (opcode)
    {
    case 0x00:
        DISASM("nop");
        break;

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
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_b));

        DISASM("ld (hl), b");
        break;

    case 0x71:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_c));

        DISASM("ld (hl), c");
        break;

    case 0x72:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_d));

        DISASM("ld (hl), d");
        break;

    case 0x73:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_e));

        DISASM("ld (hl), e");
        break;

    case 0x74:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_h));

        DISASM("ld (hl), h");
        break;

    case 0x75:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_l));

        DISASM("ld (hl), l");
        break;

    case 0x77:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_a));

        DISASM("ld (hl), a");
        break;
#pragma endregion

    // ld r8, (hl)
#pragma region
    case 0x46:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_b));

        DISASM("ld b, (hl)");
        break;

    case 0x4E:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_c));

        DISASM("ld c, (hl)");
        break;

    case 0x56:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_d));

        DISASM("ld d, (hl)");
        break;

    case 0x5E:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_e));

        DISASM("ld e, (hl)");
        break;

    case 0x66:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_h));

        DISASM("ld h, (hl)");
        break;

    case 0x6E:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_l));

        DISASM("ld l, (hl)");
        break;

    case 0x7E:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_a));

        DISASM("ld a, (hl)");
        break;

#pragma endregion

    // ld r8, imm8
#pragma region
    case 0x06:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_b = imm_val8;

        DISASM("ld b, 0x%02x", imm_val8);
        break;

    case 0x0E:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_c = imm_val8;

        DISASM("ld c, 0x%02x", imm_val8);
        break;

    case 0x16:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_d = imm_val8;

        DISASM("ld d, 0x%02x", imm_val8);
        break;

    case 0x1E:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_e = imm_val8;

        DISASM("ld e, 0x%02x", imm_val8);
        break;

    case 0x26:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_h = imm_val8;

        DISASM("ld h, 0x%02x", imm_val8);
        break;

    case 0x2E:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_l = imm_val8;

        DISASM("ld l, 0x%02x", imm_val8);
        break;

    case 0x3E:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_a = imm_val8;

        DISASM("ld a, 0x%02x", imm_val8);
        break;
#pragma endregion

    // etc 8-bit loads
#pragma region
    case 0x02:
        GBCHK(cpu_mem_write(cpu, cpu->reg_bc, cpu->reg_a));

        DISASM("ld (bc), a");
        break;

    case 0x12:
        GBCHK(cpu_mem_write(cpu, cpu->reg_de, cpu->reg_a));
        
        DISASM("ld (de), a");
        break;

    case 0x22:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_a));
        cpu->reg_hl++;
        
        DISASM("ld (hl++), a");
        break;

    case 0x32:
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, cpu->reg_a));
        cpu->reg_hl--;
        
        DISASM("ld (hl--), a");
        break;

    case 0x0A:
        GBCHK(cpu_mem_read(cpu, cpu->reg_bc, &cpu->reg_a));

        DISASM("ld a, (bc)");
        break;

    case 0x1A:
        GBCHK(cpu_mem_read(cpu, cpu->reg_de, &cpu->reg_a));

        DISASM("ld a, (de)");
        break;

    case 0x2A:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_a));
        cpu->reg_hl++;

        DISASM("ld a, (hl++)");
        break;

    case 0x3A:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_a));
        cpu->reg_hl--;

        DISASM("ld a, (hl--)");
        break;

    case 0x36:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, imm_val8));

        DISASM("ld (hl), 0x%02x", imm_val8);
        break;

    case 0xE0:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        GBCHK(cpu_mem_write(cpu, 0xFF00 + imm_val8, cpu->reg_a));
        
        DISASM("ld (0xFF00 + 0x%02x), a", imm_val8);
        break;

    case 0xF0:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        GBCHK(cpu_mem_read(cpu, 0xFF00 + imm_val8, &cpu->reg_a));

        DISASM("ld a, (0xFF00 + 0x%02x)", imm_val8);
        break;

    case 0xE2:
        GBCHK(cpu_mem_write(cpu, 0xFF00 + cpu->reg_c, cpu->reg_a));
        
        DISASM("ld (0xFF00 + c), a");
        break;

    case 0xF2:
        GBCHK(cpu_mem_read(cpu, 0xFF00 + cpu->reg_c, &cpu->reg_a));

        DISASM("ld a, (0xFF00 + c)");
        break;

    case 0xEA:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        GBCHK(cpu_mem_write(cpu, imm_val16, cpu->reg_a));

        DISASM("ld (0x%04x), a", imm_val16);
        break;

    case 0xFA:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        GBCHK(cpu_mem_read(cpu, imm_val16, &cpu->reg_a));

        DISASM("ld a, (0x%04x)", imm_val16);
        break;
#pragma endregion
#pragma endregion

    // 16-bit loads
#pragma region
    // ld r16, imm16
#pragma region
    case 0x01:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        cpu->reg_bc = imm_val16;

        DISASM("ld bc, 0x%04x", imm_val16);
        break;

    case 0x11:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        cpu->reg_de = imm_val16;

        DISASM("ld de, 0x%04x", imm_val16);
        break;

    case 0x21:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        cpu->reg_hl = imm_val16;

        DISASM("ld hl, 0x%04x", imm_val16);
        break;

    case 0x31:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        cpu->sp = imm_val16;

        DISASM("ld sp, 0x%04x", imm_val16);
        break;
#pragma endregion

    // pop r16
#pragma region
    case 0xC1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_bc = imm_val16;

        DISASM("pop bc");
        break;

    case 0xD1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_de = imm_val16;

        DISASM("pop de");
        break;

    case 0xE1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_hl = imm_val16;

        DISASM("pop hl");
        break;

    case 0xF1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_af = imm_val16;
        cpu->reg_f &= 0xf0; // least 4 bits of the flags register must be always zero

        DISASM("pop af");
        break;
#pragma endregion

    // push r16
#pragma region
    case 0xC5:
        sync_with_cpu(cpu->gb, 4); // internal timing

        cpu->sp -= 2;
        GBCHK(cpu_mem_write_word(cpu, cpu->sp, cpu->reg_bc));

        DISASM("push bc");
        break;

    case 0xD5:
        sync_with_cpu(cpu->gb, 4); // internal timing

        cpu->sp -= 2;
        GBCHK(cpu_mem_write_word(cpu, cpu->sp, cpu->reg_de));

        DISASM("push de");
        break;

    case 0xE5:
        sync_with_cpu(cpu->gb, 4); // internal timing
        
        cpu->sp -= 2;
        GBCHK(cpu_mem_write_word(cpu, cpu->sp, cpu->reg_hl));

        DISASM("push hl");
        break;

    case 0xF5:
        sync_with_cpu(cpu->gb, 4); // internal timing
        
        cpu->sp -= 2;
        GBCHK(cpu_mem_write_word(cpu, cpu->sp, cpu->reg_af));

        DISASM("push af");
        break;
#pragma endregion

    // etc 16-bit loads
#pragma region
    case 0x08:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        GBCHK(cpu_mem_write_word(cpu, imm_val16, cpu->sp));
        DISASM("ld (0x%04x), sp", imm_val16);
        break;

    case 0xF9:
        cpu->sp = cpu->reg_hl;
        sync_with_cpu(cpu->gb, 4); // internal timing

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
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_add(cpu, imm_val8);

        DISASM("add a, (hl)");
        break;

    case 0x96:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_sub(cpu, imm_val8);

        DISASM("sub a, (hl)");
        break;

    case 0xA6:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_and(cpu, imm_val8);

        DISASM("and a, (hl)");
        break;

    case 0xB6:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_or(cpu, imm_val8);

        DISASM("or a, (hl)");
        break;

    case 0x8E:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_adc(cpu, imm_val8);

        DISASM("adc a, (hl)");
        break;

    case 0x9E:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_sbc(cpu, imm_val8);

        DISASM("sbc a, (hl)");
        break;

    case 0xAE:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_xor(cpu, imm_val8);

        DISASM("xor a, (hl)");
        break;

    case 0xBE:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_cp(cpu, imm_val8);

        DISASM("cp a, (hl)");
        break;
#pragma endregion

    // OP a, imm8
#pragma region
    case 0xC6:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_add(cpu, imm_val8);

        DISASM("add a, 0x%02x", imm_val8);
        break;

    case 0xD6:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_sub(cpu, imm_val8);

        DISASM("sub a, 0x%02x", imm_val8);
        break;

    case 0xE6:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_and(cpu, imm_val8);

        DISASM("and a, 0x%02x", imm_val8);
        break;

    case 0xF6:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_or(cpu, imm_val8);

        DISASM("or a, 0x%02x", imm_val8);
        break;

    case 0xCE:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_adc(cpu, imm_val8);

        DISASM("adc a, 0x%02x", imm_val8);
        break;

    case 0xDE:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_sbc(cpu, imm_val8);

        DISASM("sbc a, 0x%02x", imm_val8);
        break;

    case 0xEE:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_xor(cpu, imm_val8);

        DISASM("xor a, 0x%02x", imm_val8);
        break;

    case 0xFE:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        cpu_instr_cp(cpu, imm_val8);

        DISASM("cp a, 0x%02x", imm_val8);
        break;
#pragma endregion

    // misc 8-bit arithmetic
#pragma region
    case 0x34:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_inc(cpu, &imm_val8);
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, imm_val8));

        DISASM("inc (hl)");
        break;

    case 0x35:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &imm_val8));
        cpu_instr_dec(cpu, &imm_val8);
        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, imm_val8));
        
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

#pragma endregion
#pragma endregion
    }
}