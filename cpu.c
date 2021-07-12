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

    case 0x4a:
        cpu->reg_c = cpu->reg_d;

        DISASM("ld c, d");
        break;

    case 0x4b:
        cpu->reg_c = cpu->reg_e;

        DISASM("ld c, e");
        break;

    case 0x4c:
        cpu->reg_c = cpu->reg_h;

        DISASM("ld c, h");
        break;

    case 0x4d:
        cpu->reg_c = cpu->reg_l;

        DISASM("ld c, l");
        break;

    case 0x4f:
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

    case 0x5a:
        cpu->reg_e = cpu->reg_d;

        DISASM("ld e, d");
        break;

    case 0x5b:
        cpu->reg_e = cpu->reg_e;

        DISASM("ld e, e");
        break;

    case 0x5c:
        cpu->reg_e = cpu->reg_h;

        DISASM("ld e, h");
        break;

    case 0x5d:
        cpu->reg_e = cpu->reg_l;

        DISASM("ld e, l");
        break;

    case 0x5f:
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

    case 0x6a:
        cpu->reg_l = cpu->reg_d;

        DISASM("ld l, d");
        break;

    case 0x6b:
        cpu->reg_l = cpu->reg_e;

        DISASM("ld l, e");
        break;

    case 0x6c:
        cpu->reg_l = cpu->reg_h;

        DISASM("ld l, h");
        break;

    case 0x6d:
        cpu->reg_l = cpu->reg_l;

        DISASM("ld l, l");
        break;

    case 0x6f:
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

    case 0x7a:
        cpu->reg_a = cpu->reg_d;

        DISASM("ld a, d");
        break;

    case 0x7b:
        cpu->reg_a = cpu->reg_e;

        DISASM("ld a, e");
        break;

    case 0x7c:
        cpu->reg_a = cpu->reg_h;

        DISASM("ld a, h");
        break;

    case 0x7d:
        cpu->reg_a = cpu->reg_l;

        DISASM("ld a, l");
        break;

    case 0x7f:
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

    case 0x4e:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_c));

        DISASM("ld c, (hl)");
        break;

    case 0x56:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_d));

        DISASM("ld d, (hl)");
        break;

    case 0x5e:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_e));

        DISASM("ld e, (hl)");
        break;

    case 0x66:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_h));

        DISASM("ld h, (hl)");
        break;

    case 0x6e:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_l));

        DISASM("ld l, (hl)");
        break;

    case 0x7e:
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

    case 0x0e:
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

    case 0x1e:
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

    case 0x2e:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;
        cpu->reg_l = imm_val8;

        DISASM("ld l, 0x%02x", imm_val8);
        break;

    case 0x3e:
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

    case 0x0a:
        GBCHK(cpu_mem_read(cpu, cpu->reg_bc, &cpu->reg_a));

        DISASM("ld a, (bc)");
        break;

    case 0x1a:
        GBCHK(cpu_mem_read(cpu, cpu->reg_de, &cpu->reg_a));

        DISASM("ld a, (de)");
        break;

    case 0x2a:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_a));
        cpu->reg_hl++;

        DISASM("ld a, (hl++)");
        break;

    case 0x3a:
        GBCHK(cpu_mem_read(cpu, cpu->reg_hl, &cpu->reg_a));
        cpu->reg_hl--;

        DISASM("ld a, (hl--)");
        break;

    case 0x36:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        GBCHK(cpu_mem_write(cpu, cpu->reg_hl, imm_val8));

        DISASM("ld (hl), 0x%02x");
        break;

    case 0xe0:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        GBCHK(cpu_mem_write(cpu, 0xFF00 + imm_val8, cpu->reg_a));
        
        DISASM("ld (0xFF00 + 0x%02x), a", imm_val8);
        break;

    case 0xf0:
        GBCHK(cpu_mem_read(cpu, cpu->pc, &imm_val8));
        cpu->pc++;

        GBCHK(cpu_mem_read(cpu, 0xFF00 + imm_val8, &cpu->reg_a));

        DISASM("ld a, (0xFF00 + 0x%02x)", imm_val8);
        break;

    case 0xe2:
        GBCHK(cpu_mem_write(cpu, 0xFF00 + cpu->reg_c, cpu->reg_a));
        
        DISASM("ld (0xFF00 + c), a");
        break;

    case 0xf2:
        GBCHK(cpu_mem_read(cpu, 0xFF00 + cpu->reg_c, &cpu->reg_a));

        DISASM("ld a, (0xFF00 + c)");
        break;

    case 0xea:
        GBCHK(cpu_mem_read_word(cpu, cpu->pc, &imm_val16));
        cpu->pc += 2;

        GBCHK(cpu_mem_write(cpu, imm_val16, cpu->reg_a));

        DISASM("ld (0x%04x), a", imm_val16);
        break;

    case 0xfa:
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
    case 0xc1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_bc = imm_val16;

        DISASM("pop bc");
        break;

    case 0xd1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_de = imm_val16;

        DISASM("pop de");
        break;

    case 0xe1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_hl = imm_val16;

        DISASM("pop hl");
        break;

    case 0xf1:
        GBCHK(cpu_mem_read_word(cpu, cpu->sp, &imm_val16));
        cpu->sp += 2;

        cpu->reg_af = imm_val16;
        cpu->reg_f &= 0xf0; // least 4 bits of the flags register must be always zero

        DISASM("pop af");
        break;
#pragma endregion

    // push r16
#pragma region
    case 0xc5:
        sync_with_cpu(cpu->gb, 4); // internal timing

        cpu->sp -= 2;
        GBCHK(cpu_mem_write_word(cpu, cpu->sp, cpu->reg_bc));

        DISASM("push bc");
        break;

    case 0xd5:
        sync_with_cpu(cpu->gb, 4); // internal timing

        cpu->sp -= 2;
        GBCHK(cpu_mem_write_word(cpu, cpu->sp, cpu->reg_de));

        DISASM("push de");
        break;

    case 0xe5:
        sync_with_cpu(cpu->gb, 4); // internal timing
        
        cpu->sp -= 2;
        GBCHK(cpu_mem_write_word(cpu, cpu->sp, cpu->reg_hl));

        DISASM("push hl");
        break;

    case 0xf5:
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

    case 0xf9:
        cpu->sp = cpu->reg_hl;
        sync_with_cpu(cpu->gb, 4); // internal timing

        DISASM("ld sp, hl");
        break;
#pragma endregion
#pragma endregion
    }
}