/*
 * This file is part of the orion128-core distribution
 * (https://gitlab.romanchenko.su/esp/components/orion128-core.git).
 * Copyright (c) 2022 Dmitry Romanchenko.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */#include <string.h>

#include "esp_log.h"
#include "computer.h"
#include "cpu.h"

#ifdef CPU_CYCLES_ENABLE
#include "esp_timer.h"
#endif

#ifndef NDEBUG
#define __CPU_INLINE__
#else
#define __CPU_INLINE__ inline
#endif

#define CPU_REG_B 0
#define CPU_REG_C 1
#define CPU_REG_D 2
#define CPU_REG_E 3
#define CPU_REG_H 4
#define CPU_REG_L 5
#define CPU_REG_M 6
#define CPU_REG_A 7
#define CPU_REG_SIZE 8

#define CPU_FILE_B 1
#define CPU_FILE_C 0
#define CPU_FILE_D 3
#define CPU_FILE_E 2
#define CPU_FILE_H 5
#define CPU_FILE_L 4
#define CPU_FLAGS  6
#define CPU_FILE_A 7
#define CPU_REG_FILE_SIZE 8

#define CPU_REG_BC 0
#define CPU_REG_DE 1
#define CPU_REG_HL 2
#define CPU_REG_SP 3
#define CPU_PAIR_SIZE 4

#define CPU_FILE_BC 0
#define CPU_FILE_DE 2
#define CPU_FILE_HL 4
#define CPU_FILE_PSW 6

#define CPU_FLAG_C  0
#define CPU_FLAG_P  2
#define CPU_FLAG_AC 4
#define CPU_FLAG_Z  6
#define CPU_FLAG_S  7

#define CPU_MASK_C  0x01
#define CPU_MASK_P  0x04
#define CPU_MASK_AC 0x10
#define CPU_MASK_Z  0x40
#define CPU_MASK_S  0x80

#define CPU_HL_VAL(cpu)  (*((uint16_t *)&cpu->reg_file[CPU_FILE_L]))
#define CPU_PSW_VAL(cpu) (*((uint16_t *)&cpu->reg_file[CPU_FILE_A]))
#define CPU_A_VAL(cpu)   (cpu->reg_file[CPU_FILE_A])

#define CPU_IS_SET_FLAG_C(cpu)  (cpu->reg_file[CPU_FLAGS] & CPU_MASK_C)
#define CPU_IS_SET_FLAG_P(cpu)  (cpu->reg_file[CPU_FLAGS] & CPU_MASK_P)
#define CPU_IS_SET_FLAG_AC(cpu) (cpu->reg_file[CPU_FLAGS] & CPU_MASK_AC)
#define CPU_IS_SET_FLAG_Z(cpu)  (cpu->reg_file[CPU_FLAGS] & CPU_MASK_Z)
#define CPU_IS_SET_FLAG_S(cpu)  (cpu->reg_file[CPU_FLAGS] & CPU_MASK_S)

#define CPU_SET_FLAG_C(cpu)  (cpu->reg_file[CPU_FLAGS] |= CPU_MASK_C)
#define CPU_SET_FLAG_P(cpu)  (cpu->reg_file[CPU_FLAGS] |= CPU_MASK_P)
#define CPU_SET_FLAG_AC(cpu) (cpu->reg_file[CPU_FLAGS] |= CPU_MASK_AC)
#define CPU_SET_FLAG_Z(cpu)  (cpu->reg_file[CPU_FLAGS] |= CPU_MASK_Z)
#define CPU_SET_FLAG_S(cpu)  (cpu->reg_file[CPU_FLAGS] |= CPU_MASK_S)

#define CPU_CLR_FLAG_C(cpu)  (cpu->reg_file[CPU_FLAGS] &= ~CPU_MASK_C)
#define CPU_CLR_FLAG_P(cpu)  (cpu->reg_file[CPU_FLAGS] &= ~CPU_MASK_P)
#define CPU_CLR_FLAG_AC(cpu) (cpu->reg_file[CPU_FLAGS] &= ~CPU_MASK_AC)
#define CPU_CLR_FLAG_Z(cpu)  (cpu->reg_file[CPU_FLAGS] &= ~CPU_MASK_Z)
#define CPU_CLR_FLAG_S(cpu)  (cpu->reg_file[CPU_FLAGS] &= ~CPU_MASK_S)

static const char *TAG = "CPU";

#ifdef CPU_MNEMONIC_ENABLE
static char cpu_mnemonic[32];

static const char *cpu_pairs_name[4] = {
    "BC", "DE", "HL", "SP"
};
static const char *cpu_regs_name[8] = {
    "B", "C", "D", "E", "H", "L", "M", "A"
};
static const char *cpu_conds_name[8] = {
    "NZ", "Z", "NC", "C", "PO", "PE", "P", "M"
};
#endif

#ifdef CPU_CYCLES_ENABLE
static const uint8_t cpu_cycles_num[] = {
//  0   1   2   3   4   5   6   7
    4,  10, 7,  5,  5,  5,  7,  4,  // 0.0
    4,  10, 7,  5,  5,  5,  7,  4,  // 0.1
    4,  10, 7,  5,  5,  5,  7,  4,  // 0.2
    4,  10, 7,  5,  5,  5,  7,  4,  // 0.3
    4,  10, 16, 5,  5,  5,  7,  4,  // 0.4
    4,  10, 16, 5,  5,  5,  7,  4,  // 0.5
    4,  10, 13, 5,  10, 10, 10, 4,  // 0.6
    4,  10, 13, 5,  5,  5,  7,  4,  // 0.7

    5,  5,  5,  5,  5,  5,  7,  5,  // 1.0
    5,  5,  5,  5,  5,  5,  7,  5,  // 1.1
    5,  5,  5,  5,  5,  5,  7,  5,  // 1.2
    5,  5,  5,  5,  5,  5,  7,  5,  // 1.3
    5,  5,  5,  5,  5,  5,  7,  5,  // 1.4
    5,  5,  5,  5,  5,  5,  7,  5,  // 1.5
    7,  7,  7,  7,  7,  7,  7,  7,  // 1.6
    5,  5,  5,  5,  5,  5,  7,  5,  // 1.7

    4,  4,  4,  4,  4,  4,  7,  4,  // 2.0
    4,  4,  4,  4,  4,  4,  7,  4,  // 2.1
    4,  4,  4,  4,  4,  4,  7,  4,  // 2.2
    4,  4,  4,  4,  4,  4,  7,  4,  // 2.3
    4,  4,  4,  4,  4,  4,  7,  4,  // 2.4
    4,  4,  4,  4,  4,  4,  7,  4,  // 2.5
    4,  4,  4,  4,  4,  4,  7,  4,  // 2.6
    4,  4,  4,  4,  4,  4,  7,  4,  // 2.7

    5,  10, 10, 10, 11, 11, 7,  11, // 3.0
    5,  10, 10, 10, 11, 11, 7,  11, // 3.1
    5,  10, 10, 10, 11, 11, 7,  11, // 3.2
    5,  10, 10, 10, 11, 11, 7,  11, // 3.3
    5,  10, 10, 18, 11, 11, 7,  11, // 3.4
    5,  5,  10, 5,  11, 11, 7,  11, // 3.5
    5,  10, 10, 4,  11, 11, 7,  11, // 3.6
    5,  5,  10, 4,  11, 11, 7,  11  // 3.7
};
#endif

static const uint8_t cpu_parity[256] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 
};

#ifdef CPU_CYCLES_ENABLE
__CPU_INLINE__ uint32_t cpu_time()
{
    return esp_timer_get_time() & 0xffff;
}
#endif

esp_err_t cpu_create(cpu_t **pcpu)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(pcpu ? ESP_OK : ESP_ERR_INVALID_ARG);
    cpu_t *cpu = (cpu_t *)malloc(sizeof(cpu_t));
    ESP_ERROR_CHECK(cpu ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(cpu, sizeof(cpu_t));

    // CPU_REG_FILE_SIZE = 8
    cpu->reg_file = (uint8_t *)malloc(CPU_REG_FILE_SIZE * sizeof(uint8_t));
    ESP_ERROR_CHECK(cpu->reg_file ? ESP_OK : ESP_ERR_NO_MEM);
    // CPU_PAIR_SIZE = 4
    cpu->pair = (uint16_t **)malloc(CPU_PAIR_SIZE * sizeof(uint16_t *));
    ESP_ERROR_CHECK(cpu->pair ? ESP_OK : ESP_ERR_NO_MEM);
    // CPU_PAIR_SIZE = 4
    cpu->push_pop_pair = (uint16_t **)malloc(CPU_PAIR_SIZE * sizeof(uint16_t *));
    ESP_ERROR_CHECK(cpu->push_pop_pair ? ESP_OK : ESP_ERR_NO_MEM);
    // CPU_REG_SIZE = 8
    cpu->reg = (uint8_t **)malloc(CPU_REG_SIZE * sizeof(uint8_t *));
    ESP_ERROR_CHECK(cpu->reg ? ESP_OK : ESP_ERR_NO_MEM);

    *pcpu = cpu;
    return r;
}

esp_err_t cpu_init(cpu_t *cpu)
{
    uint16_t *cpu_pair[CPU_PAIR_SIZE] = {
        (uint16_t *)&cpu->reg_file[CPU_FILE_BC],
        (uint16_t *)&cpu->reg_file[CPU_FILE_DE],
        (uint16_t *)&cpu->reg_file[CPU_FILE_HL],
        &cpu->sp
    };
    uint16_t *cpu_push_pop_pair[CPU_PAIR_SIZE] = {
        (uint16_t *)&cpu->reg_file[CPU_FILE_BC],
        (uint16_t *)&cpu->reg_file[CPU_FILE_DE],
        (uint16_t *)&cpu->reg_file[CPU_FILE_HL],
        (uint16_t *)&cpu->reg_file[CPU_FILE_PSW]
    };
    uint8_t *cpu_reg[CPU_REG_SIZE] = {
        (uint8_t *)&cpu->reg_file[CPU_FILE_B],
        (uint8_t *)&cpu->reg_file[CPU_FILE_C],
        (uint8_t *)&cpu->reg_file[CPU_FILE_D],
        (uint8_t *)&cpu->reg_file[CPU_FILE_E],
        (uint8_t *)&cpu->reg_file[CPU_FILE_H],
        (uint8_t *)&cpu->reg_file[CPU_FILE_L],
        (uint8_t *)&cpu->reg_file[CPU_FILE_A],
        (uint8_t *)&cpu->reg_file[CPU_FILE_A]
    };

    ESP_ERROR_CHECK(cpu->reader ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(cpu->writer ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(cpu->memory ? ESP_OK : ESP_ERR_INVALID_STATE);

    memcpy(cpu->pair, cpu_pair, sizeof(cpu_pair));
    memcpy(cpu->push_pop_pair, cpu_push_pop_pair, sizeof(cpu_push_pop_pair));
    memcpy(cpu->reg, cpu_reg, sizeof(cpu_reg));

    return cpu_reset(cpu);
}

esp_err_t cpu_done(cpu_t *cpu)
{
    ESP_ERROR_CHECK(cpu ? ESP_OK : ESP_ERR_INVALID_ARG);
    free(cpu->reg_file);
    free(cpu->pair);
    free(cpu->push_pop_pair);
    free(cpu->reg);

    free(cpu);

    return ESP_OK;
}
esp_err_t cpu_reset(cpu_t *cpu) {
    bzero(cpu->reg_file, CPU_REG_FILE_SIZE * sizeof(uint8_t));

    cpu->pc = 0;
    cpu->sp = 0;
    cpu->is_word = 0;
    cpu->cmd = 0;
#ifdef CPU_CYCLES_ENABLE
    cpu->cycles = 0;
    cpu->us_timer = cpu_time();
    cpu->speed = 0;
    cpu->steps = 0;
#endif
#ifdef CPU_MNEMONIC_ENABLE
    cpu->save_pc = 0;
    cpu->invalid_op = 0;
#endif
    return ESP_OK;
}


__CPU_INLINE__ const uint8_t *cpu_get_read_mem_ptr(cpu_t *cpu, uint16_t addr) {
    return cpu->reader(addr, cpu->memory);
}

__CPU_INLINE__ uint8_t *cpu_get_write_mem_ptr(cpu_t *cpu, uint16_t addr) {
    return cpu->writer(addr, cpu->memory);
}

__CPU_INLINE__ const uint8_t *cpu_get_src_ptr(cpu_t *cpu, uint32_t reg_idx) {
    if (reg_idx == CPU_REG_M)
        return cpu_get_read_mem_ptr(cpu, CPU_HL_VAL(cpu));
    else
        return cpu->reg[reg_idx];
}

__CPU_INLINE__ uint8_t *cpu_get_dst_ptr(cpu_t *cpu, uint32_t reg_idx) {
    if (reg_idx == CPU_REG_M) {
        return cpu_get_write_mem_ptr(cpu, CPU_HL_VAL(cpu));
    }
    else {
        return cpu->reg[reg_idx];
    }
}

__CPU_INLINE__ uint8_t cpu_get_condition(cpu_t *cpu, uint32_t dst_idx) {
    switch(dst_idx) {
        case 0x00:
            return !CPU_IS_SET_FLAG_Z(cpu);
        case 0x01:
            return CPU_IS_SET_FLAG_Z(cpu);
        case 0x02:
            return !CPU_IS_SET_FLAG_C(cpu);
        case 0x03:
            return CPU_IS_SET_FLAG_C(cpu);
        case 0x04:
            return !CPU_IS_SET_FLAG_P(cpu);
        case 0x05:
            return CPU_IS_SET_FLAG_P(cpu);
        case 0x06:
            return !CPU_IS_SET_FLAG_S(cpu);
        case 0x07:
            return CPU_IS_SET_FLAG_S(cpu);
    }
    return 0;
}

__CPU_INLINE__ void cpu_update_flags_zsp(cpu_t *cpu, uint8_t val) {
    if (val) CPU_CLR_FLAG_Z(cpu);
    else CPU_SET_FLAG_Z(cpu);
    if (val & 0x80) CPU_SET_FLAG_S(cpu);
    else CPU_CLR_FLAG_S(cpu);
    if (cpu_parity[val]) CPU_SET_FLAG_P(cpu);
    else CPU_CLR_FLAG_P(cpu);
}

__CPU_INLINE__ void cpu_update_flags_zsp_def(cpu_t *cpu) {
    cpu_update_flags_zsp(cpu, CPU_A_VAL(cpu));
}

__CPU_INLINE__ void cpu_update_flag_c(cpu_t *cpu, uint16_t val) {
    if (val) CPU_SET_FLAG_C(cpu);
    else CPU_CLR_FLAG_C(cpu);
}

__CPU_INLINE__ void cpu_update_flag_ac(cpu_t *cpu, uint8_t val) {
    if (val) CPU_SET_FLAG_AC(cpu);
    else CPU_CLR_FLAG_AC(cpu);
}


// 00000000
__CPU_INLINE__ void cpu_cmd_nop(cpu_t *cpu) {
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "NOP           ");
#endif
}

// 00rp0001
__CPU_INLINE__ void cpu_cmd_lxi(cpu_t *cpu, uint32_t rp_idx) {
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
    *(cpu->pair[rp_idx]) = val;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "LXI %s, 0%04xH", cpu_pairs_name[rp_idx], val);
#endif
}

// 00rp1001
__CPU_INLINE__ void cpu_cmd_dad(cpu_t *cpu, uint32_t idx) {
    uint16_t *phl = cpu->pair[CPU_REG_HL];
    uint16_t *prp = cpu->pair[idx];
    uint32_t res = *phl + *prp;
    *phl = res;
    cpu_update_flag_c(cpu, res & 0x10000);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "DAD %s        ", cpu_pairs_name[idx]);
#endif
}

// 000r0010
__CPU_INLINE__ void cpu_cmd_stax(cpu_t *cpu, uint32_t idx) {
    uint16_t *prp = cpu->pair[idx];
    const uint8_t *psr = cpu_get_src_ptr(cpu, CPU_REG_A);
    uint8_t *pdr = cpu_get_write_mem_ptr(cpu, *prp);
    *pdr = *psr;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "STAX %s       ", cpu_pairs_name[idx]);
#endif
}

// 000r1010
__CPU_INLINE__ void cpu_cmd_ldax(cpu_t *cpu, uint32_t idx) {
    uint16_t *prp = cpu->pair[idx];
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    const uint8_t *psr = cpu_get_read_mem_ptr(cpu, *prp);
    *pdr = *psr;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "LDAX %s       ", cpu_pairs_name[idx]);
#endif
}

// 00100010
__CPU_INLINE__ void cpu_cmd_shld(cpu_t *cpu) {
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
    uint16_t *phl = cpu->pair[CPU_REG_HL];
    uint16_t *pdr = (uint16_t *)cpu_get_write_mem_ptr(cpu, val);
    *pdr = *phl;
    cpu->is_word = 1;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "SHLD         ");
#endif
}

// 00101010
__CPU_INLINE__ void cpu_cmd_lhld(cpu_t *cpu) {
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
    uint16_t *phl = cpu->pair[CPU_REG_HL];
    uint16_t *psr = (uint16_t *)cpu_get_read_mem_ptr(cpu, val);
    *phl = *psr;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "LHLD          ");
#endif
}

// 00110010
__CPU_INLINE__ void cpu_cmd_sta(cpu_t *cpu) {
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
    const uint8_t *psr = cpu_get_src_ptr(cpu, CPU_REG_A);
    uint8_t *pdr = cpu_get_write_mem_ptr(cpu, val);
    *pdr = *psr;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "STA 0%04xH    ", val);
#endif
}

// 00111010
__CPU_INLINE__ void cpu_cmd_lda(cpu_t *cpu) {
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    const uint8_t *psr = cpu_get_read_mem_ptr(cpu, val);
    *pdr = *psr;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "LDA 0%04xH    ", val);
#endif
}

// 00rp0011
__CPU_INLINE__ void cpu_cmd_inx(cpu_t *cpu, uint32_t rp_idx) {
    uint16_t *prp = cpu->pair[rp_idx];
    ++(*prp);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "INX %s        ", cpu_pairs_name[rp_idx]);
#endif
}

// 00rp10011
__CPU_INLINE__ void cpu_cmd_dcx(cpu_t *cpu, uint32_t rp_idx) {
    uint16_t *prp = cpu->pair[rp_idx];
    --(*prp);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "DCX %s        ", cpu_pairs_name[rp_idx]);
#endif
}

// 00ddd100
__CPU_INLINE__ void cpu_cmd_inr(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, idx);
    *pdr = *psr + 1;
    cpu_update_flags_zsp(cpu, *pdr);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "INR %s         ", cpu_regs_name[idx]);
#endif
}

// 00ddd101
__CPU_INLINE__ void cpu_cmd_dcr(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, idx);
    *pdr = *psr - 1;
    cpu_update_flags_zsp(cpu, *pdr);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "DCR %s         ", cpu_regs_name[idx]);
#endif
}

// 00ddd110
__CPU_INLINE__ void cpu_cmd_mvi(cpu_t *cpu, uint32_t idx) {
    uint8_t val = *cpu_get_read_mem_ptr(cpu, cpu->pc++);
    *cpu_get_dst_ptr(cpu, idx) = val;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "MVI %s, 0%02xH   ", cpu_regs_name[idx], val);
#endif
}

// 00000111
__CPU_INLINE__ void cpu_cmd_rlc(cpu_t *cpu) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, CPU_REG_A);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint8_t c = *psr & 0x80;
    *pdr = (*psr << 1) | (c ? 1 : 0);
    cpu_update_flag_c(cpu, c);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "RLC           ");
#endif
}

// 00001111
__CPU_INLINE__ void cpu_cmd_rrc(cpu_t *cpu) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, CPU_REG_A);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint8_t c = *psr & 0x01;
    *pdr = (*psr >> 1) | (c ? 0x80 : 0);
    cpu_update_flag_c(cpu, c);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "RRC           ");
#endif
}

// 00010111
__CPU_INLINE__ void cpu_cmd_ral(cpu_t *cpu) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, CPU_REG_A);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint8_t c = *psr & 0x80;
    *pdr = (*psr << 1) | (CPU_IS_SET_FLAG_C(cpu) ? 1 : 0);
    cpu_update_flag_c(cpu, c);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "RAL           ");
#endif
}

// 00011111
__CPU_INLINE__ void cpu_cmd_rar(cpu_t *cpu) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, CPU_REG_A);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint8_t c = *psr & 0x01;
    *pdr = (*psr >> 1) | (CPU_IS_SET_FLAG_C(cpu) ? 0x80 : 0);
    cpu_update_flag_c(cpu, c);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "RAR           ");
#endif
}

// 00100111
__CPU_INLINE__ void cpu_cmd_daa(cpu_t *cpu) {
    if ((CPU_A_VAL(cpu) & 0x0f) > 0x09 || CPU_IS_SET_FLAG_AC(cpu)) {
        CPU_A_VAL(cpu) += 0x09;
    }
    else if ((CPU_A_VAL(cpu) & 0xf0) > 0x90 || CPU_IS_SET_FLAG_AC(cpu)) {
        CPU_A_VAL(cpu) += 0x90;
    }
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "DAA           ");
#endif
    ESP_LOGD(TAG, "DAA instruction on address: 0x%04x", cpu->pc);
}

// 00101111
__CPU_INLINE__ void cpu_cmd_cma(cpu_t *cpu) {
    CPU_A_VAL(cpu) ^= 0xff;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "CMA           ");
#endif
}

// 00110111
__CPU_INLINE__ void cpu_cmd_stc(cpu_t *cpu) {
    CPU_SET_FLAG_C(cpu);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "STC           ");
#endif
}

// 00111111
__CPU_INLINE__ void cpu_cmd_cmc(cpu_t *cpu) {
    cpu->reg_file[CPU_FLAGS] ^= CPU_MASK_C;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "CMC           ");
#endif
}

// 01dddsss
__CPU_INLINE__ void cpu_cmd_mov(cpu_t *cpu, uint32_t dst_idx, uint32_t src_idx) {
    *cpu_get_dst_ptr(cpu, dst_idx) = *cpu_get_src_ptr(cpu, src_idx);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "MOV %s, %s      ", cpu_regs_name[dst_idx], cpu_regs_name[src_idx]);
#endif
}

// 10000sss
__CPU_INLINE__ void cpu_cmd_add(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint16_t res = *pdr + *psr;
    *pdr = res;
    cpu_update_flags_zsp_def(cpu);
    cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ADD %s         ", cpu_regs_name[idx]);
#endif
}

// 10001sss
__CPU_INLINE__ void cpu_cmd_adc(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint16_t res = *pdr + *psr + (CPU_IS_SET_FLAG_C(cpu) ? 1 : 0);
    *pdr = res;
    cpu_update_flags_zsp_def(cpu);
    cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ADC %s         ", cpu_regs_name[idx]);
#endif
}

// 10010sss
__CPU_INLINE__ void cpu_cmd_sub(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint16_t res = *pdr - *psr;
    *pdr = res;
    cpu_update_flags_zsp_def(cpu);
    cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "SUB %s         ", cpu_regs_name[idx]);
#endif
}

// 10011sss
__CPU_INLINE__ void cpu_cmd_sbb(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint16_t res = *pdr - *psr - (CPU_IS_SET_FLAG_C(cpu) ? 1 : 0);
    *pdr = res;
    cpu_update_flags_zsp_def(cpu);
    cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "SBB %s         ", cpu_regs_name[idx]);
#endif
}

// 10100sss
__CPU_INLINE__ void cpu_cmd_ana(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    *pdr = *pdr & *psr;
    cpu_update_flags_zsp_def(cpu);
    CPU_CLR_FLAG_C(cpu);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ANA %s         ", cpu_regs_name[idx]);
#endif
}

// 10101sss
__CPU_INLINE__ void cpu_cmd_xra(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    *pdr = *pdr ^ *psr;
    cpu_update_flags_zsp_def(cpu);
    CPU_CLR_FLAG_C(cpu);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "XRA %s         ", cpu_regs_name[idx]);
#endif
}

// 10110sss
__CPU_INLINE__ void cpu_cmd_ora(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    *pdr = *pdr | *psr;
    cpu_update_flags_zsp_def(cpu);
    CPU_CLR_FLAG_C(cpu);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ORA %s         ", cpu_regs_name[idx]);
#endif
}

// 10111sss
__CPU_INLINE__ void cpu_cmd_cmp(cpu_t *cpu, uint32_t idx) {
    const uint8_t *psr = cpu_get_src_ptr(cpu, idx);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint16_t res = *pdr - *psr;
    cpu_update_flags_zsp(cpu, res);
    cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "CMP %s         ", cpu_regs_name[idx]);
#endif
}

// 10dddsss
__CPU_INLINE__ void cpu_cmd_alu(cpu_t *cpu, uint32_t dst_idx, uint32_t src_idx) {
    switch (dst_idx) {
        case 0x00:
            cpu_cmd_add(cpu, src_idx);
            break;
        case 0x01:
            cpu_cmd_adc(cpu, src_idx);
            break;
        case 0x02:
            cpu_cmd_sub(cpu, src_idx);
            break;
        case 0x03:
            cpu_cmd_sbb(cpu, src_idx);
            break;
        case 0x04:
            cpu_cmd_ana(cpu, src_idx);
            break;
        case 0x05:
            cpu_cmd_xra(cpu, src_idx);
            break;
        case 0x06:
            cpu_cmd_ora(cpu, src_idx);
            break;
        case 0x07:
            cpu_cmd_cmp(cpu, src_idx);
            break;
    }
}

// 11ddd000
__CPU_INLINE__ void cpu_cmd_r(cpu_t *cpu, uint32_t idx) {
    if (cpu_get_condition(cpu, idx)) {
        uint16_t *psp = (uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->sp);
        cpu->sp += 2;
        cpu->pc = *psp;
#ifdef CPU_CYCLES_ENABLE
        cpu->cycles += 6;
#endif
    }
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "R%s           ", cpu_conds_name[idx]);
#endif
}

// 11rp0001
__CPU_INLINE__ void cpu_cmd_pop(cpu_t *cpu, uint32_t idx) {
    uint16_t *psp = (uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->sp);
    cpu->sp += 2;
    *(cpu->push_pop_pair[idx]) = *psp;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "POP %s        ", cpu_pairs_name[idx]);
#endif
}

// 11001001
__CPU_INLINE__ void cpu_cmd_ret(cpu_t *cpu) {
    uint16_t *psp = (uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->sp);
    cpu->sp += 2;
    cpu->pc = *psp;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "RET           ");
#endif
}

// 11101001
__CPU_INLINE__ void cpu_cmd_pchl(cpu_t *cpu) {
    cpu->pc = CPU_HL_VAL(cpu);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "PCHL          ");
#endif
}

// 11111001
__CPU_INLINE__ void cpu_cmd_sphl(cpu_t *cpu) {
    cpu->sp = CPU_HL_VAL(cpu);
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "SPHL          ");
#endif
}

// 11ddd010
__CPU_INLINE__ void cpu_cmd_j(cpu_t *cpu, uint32_t idx) {
#ifdef CPU_MNEMONIC_ENABLE
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
#endif
    if (cpu_get_condition(cpu, idx)) {
#ifndef CPU_MNEMONIC_ENABLE
        uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
        cpu->pc += 2;
#endif
        cpu->pc = val;
#ifdef CPU_CYCLES_ENABLE
        cpu->cycles += 6;
#endif
    }
    else {
#ifndef CPU_MNEMONIC_ENABLE
        cpu->pc += 2;
#endif
    }
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "J%s 0%04xH    ", cpu_conds_name[idx], val);
#endif
}

// 11000011
__CPU_INLINE__ void cpu_cmd_jmp(cpu_t *cpu) {
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
    cpu->pc = val;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "JMP 0%04xH    ", val);
#endif
}

// 11010011
__CPU_INLINE__ void cpu_cmd_out(cpu_t *cpu) {
    uint8_t port = *cpu_get_read_mem_ptr(cpu, cpu->pc++);
    const uint8_t *psr = cpu_get_src_ptr(cpu, CPU_REG_A);
    uint8_t *ptr = cpu_get_write_mem_ptr(cpu, port << 8);
    *ptr = *psr;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "OUT 0%02xH      ", port);
#endif
}

// 11011011
__CPU_INLINE__ void cpu_cmd_in(cpu_t *cpu) {
    uint8_t port = *cpu_get_read_mem_ptr(cpu, cpu->pc++);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    const uint8_t *ptr = cpu_get_read_mem_ptr(cpu, port << 8);
    *pdr = *ptr;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "IN 0%02xH       ", port);
#endif
}

// 11100011
__CPU_INLINE__ void cpu_cmd_xthl(cpu_t *cpu) {
    uint16_t *psp = (uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->sp);
    uint16_t *phl = cpu->pair[CPU_REG_HL];
    uint16_t tmp = *psp;
    *psp = *phl;
    *phl = tmp;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "XTHL          ");
#endif
}

// 11101011
__CPU_INLINE__ void cpu_cmd_xchg(cpu_t *cpu) {
    uint16_t *pde = cpu->pair[CPU_REG_DE];
    uint16_t *phl = cpu->pair[CPU_REG_HL];
    uint16_t tmp = *pde;
    *pde = *phl;
    *phl = tmp;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "XCGH          ");
#endif
}

// 11110011
__CPU_INLINE__ void cpu_cmd_di(cpu_t *cpu) {
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "DI            ");
#endif
}

// 11111011
__CPU_INLINE__ void cpu_cmd_ei(cpu_t *cpu) {
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "EI            ");
#endif
}

// 11ddd100
__CPU_INLINE__ void cpu_cmd_c(cpu_t *cpu, uint32_t idx) {
#ifdef CPU_MNEMONIC_ENABLE
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
#endif
    if (cpu_get_condition(cpu, idx)) {
#ifndef CPU_MNEMONIC_ENABLE
        uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
        cpu->pc += 2;
#endif
        cpu->sp -= 2;
        uint16_t *psp = (uint16_t *)cpu_get_write_mem_ptr(cpu, cpu->sp);
        *psp = cpu->pc;
        cpu->pc = val;
        cpu->is_word = 1;
#ifdef CPU_CYCLES_ENABLE
        cpu->cycles += 6;
#endif
    }
    else {
#ifndef CPU_MNEMONIC_ENABLE
        cpu->pc += 2;
#endif
    }
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "C%s 0%04xH    ", cpu_conds_name[idx], val);
#endif
}

// 11rp0101
__CPU_INLINE__ void cpu_cmd_push(cpu_t *cpu, uint32_t idx) {
    cpu->sp -= 2;
    uint16_t *psp = (uint16_t *)cpu_get_write_mem_ptr(cpu, cpu->sp);
    *psp = *(cpu->push_pop_pair[idx]);
    cpu->is_word = 1;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "PUSH %s       ", cpu_pairs_name[idx]);
#endif
}

// 11001101
__CPU_INLINE__ void cpu_cmd_call(cpu_t *cpu) {
    uint16_t val = *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->pc);
    cpu->pc += 2;
    cpu->sp -= 2;
    uint16_t *psp = (uint16_t *)cpu_get_write_mem_ptr(cpu, cpu->sp);
    *psp = cpu->pc;
    cpu->pc = val;
    cpu->is_word = 1;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "CALL 0%04xH   ", val);
#endif
}

// 11ddd110
__CPU_INLINE__ void cpu_cmd_i(cpu_t *cpu, uint32_t idx) {
    uint8_t val = *cpu_get_read_mem_ptr(cpu, cpu->pc++);
    uint8_t *pdr = cpu_get_dst_ptr(cpu, CPU_REG_A);
    uint16_t res = 0;
    switch (idx) {
        case 0x00:
            res = *pdr + val;
            *pdr = res;
            cpu_update_flags_zsp_def(cpu);
            cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ADI 0%02xH      ", val);
#endif
            break;
        case 0x01:
            res = *pdr + val + (CPU_IS_SET_FLAG_C(cpu) ? 1 : 0);
            *pdr = res;
            cpu_update_flags_zsp_def(cpu);
            cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ACI 0%02xH      ", val);
#endif
            break;
        case 0x02:
            res = *pdr - val;
            *pdr = res;
            cpu_update_flags_zsp_def(cpu);
            cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "SUI 0%02xH      ", val);
#endif
            break;
        case 0x03:
            res = *pdr - val - (CPU_IS_SET_FLAG_C(cpu) ? 1 : 0);
            *pdr = res;
            cpu_update_flags_zsp_def(cpu);
            cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "SBI 0%02xH      ", val);
#endif
            break;
        case 0x04:
            res = *pdr & val;
            *pdr = res;
            cpu_update_flags_zsp_def(cpu);
            CPU_CLR_FLAG_C(cpu);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ANI 0%02xH      ", val);
#endif
            break;
        case 0x05:
            res = *pdr ^ val;
            *pdr = res;
            cpu_update_flags_zsp_def(cpu);
            CPU_CLR_FLAG_C(cpu);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "XRI 0%02xH      ", val);
#endif
            break;
        case 0x06:
            res = *pdr | val;
            *pdr = res;
            cpu_update_flags_zsp_def(cpu);
            CPU_CLR_FLAG_C(cpu);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "ORI 0%02xH      ", val);
#endif
            break;
        case 0x07:
            res = *pdr - val;
            cpu_update_flags_zsp(cpu, res);
            cpu_update_flag_c(cpu, res & 0x100);
#ifdef CPU_MNEMONIC_ENABLE
            sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "CPI 0%02xH      ", val);
#endif
            break;
    }
}

// 11ddd111
__CPU_INLINE__  void cpu_cmd_rst(cpu_t *cpu, uint32_t idx) {
    cpu->sp -= 2;
    uint16_t *psp = (uint16_t *)cpu_get_write_mem_ptr(cpu, cpu->sp);
    *psp = cpu->pc;
    cpu->pc = idx << 3;
    cpu->is_word = 1;
#ifdef CPU_MNEMONIC_ENABLE
    sprintf(&cpu_mnemonic[strlen(cpu_mnemonic)], "RST %d         ", idx);
#endif
}

#ifdef CPU_MNEMONIC_ENABLE
void cpu_dump_mem(cpu_t *cpu, uint32_t len, uint16_t addr) {
    char buf[64];
    uint32_t i;

    buf[0] = '\0';
    for (i = 0; i < len; ++i) {
        if ((i&7) == 0) {
            sprintf(buf, "%04x: ", addr+i);
        }
        uint8_t *p = cpu_get_read_mem_ptr(cpu, addr+i);
        sprintf(&buf[strlen(buf)], "%02x ", *p);
        if ((i&7) == 7) {
            ESP_LOGI(TAG, "%s", buf);
        }
    }
}
#endif

#ifdef CPU_MNEMONIC_ENABLE
static void cpu_trace(cpu_t *cpu) {
    if (cpu->invalid_op) {
        ESP_LOGI(TAG, "%04x: invalid instruction 0x%02x", cpu->save_pc, cpu->cmd);
    }
    else {
        ESP_LOGI(TAG, "%04x: %02x %02x %02x: %s\t// BC=%04x, DE=%04x, HL=%04x, SP=%04x, A=%02x, %c%cx%cx%cx%c [%04x %04x %04x]"
            , cpu->save_pc
            , *cpu_get_read_mem_ptr(cpu, cpu->save_pc)
            , *cpu_get_read_mem_ptr(cpu, cpu->save_pc+1)
            , *cpu_get_read_mem_ptr(cpu, cpu->save_pc+2)
            , cpu_mnemonic
            , *(cpu->pair[CPU_REG_BC])
            , *(cpu->pair[CPU_REG_DE])
            , *(cpu->pair[CPU_REG_HL])
            , *(cpu->pair[CPU_REG_SP])
            , CPU_A_VAL(cpu)
            , CPU_IS_SET_FLAG_S(cpu) ? 'S' : 's'
            , CPU_IS_SET_FLAG_Z(cpu) ? 'Z' : 'z'
            , CPU_IS_SET_FLAG_AC(cpu) ? 'A' : 'a'
            , CPU_IS_SET_FLAG_P(cpu) ? 'P' : 'p'
            , CPU_IS_SET_FLAG_C(cpu) ? 'C' : 'c'
            , *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->sp)
            , *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->sp+2)
            , *(uint16_t *)cpu_get_read_mem_ptr(cpu, cpu->sp+4)
        );
        cpu_dump_mem(32, 0xd840);
    }
}
#endif

esp_err_t cpu_step(cpu_t *cpu) {
#ifdef CPU_MNEMONIC_ENABLE
    cpu->save_pc = cpu->pc;
#endif

    const uint8_t* pc_ptr = cpu_get_read_mem_ptr(cpu, cpu->pc++);
    cpu->cmd = *pc_ptr;
    uint32_t dst_idx = (cpu->cmd >> 3) & 0x07;
    uint32_t src_idx = cpu->cmd & 0x07;
    cpu->is_word = 0;
#ifdef CPU_MNEMONIC_ENABLE
    cpu->invalid_op = 0;
    cpu_mnemonic[0] = '\0';
#endif
#ifdef CPU_CYCLES_ENABLE
    cpu->cycles += cpu_cycles_num[cpu->cmd];
#endif

    switch (cpu->cmd & 0xc0) {
        case 0x00: {
            switch (src_idx) {
                case 0:
                    if (dst_idx == 0) {
                        cpu_cmd_nop(cpu);
                        break;
                    }
#ifdef CPU_MNEMONIC_ENABLE
                    else cpu->invalid_op = 1;
#endif
                    break;
                case 1:
                    if (dst_idx & 1) {
                        cpu_cmd_dad(cpu, dst_idx>>1);
                    }
                    else {
                        cpu_cmd_lxi(cpu, dst_idx>>1);
                    }
                    break;
                case 2:
                    switch (dst_idx) {
                        case 0:
                            cpu_cmd_stax(cpu, CPU_REG_BC);
                            break;
                        case 1:
                            cpu_cmd_ldax(cpu, CPU_REG_BC);
                            break;
                        case 2:
                            cpu_cmd_stax(cpu, CPU_REG_DE);
                            break;
                        case 3:
                            cpu_cmd_ldax(cpu, CPU_REG_DE);
                            break;
                        case 4:
                            cpu_cmd_shld(cpu);
                            break;
                        case 5:
                            cpu_cmd_lhld(cpu);
                            break;
                        case 6:
                            cpu_cmd_sta(cpu);
                            break;
                        case 7:
                            cpu_cmd_lda(cpu);
                            break;
                    }
                    break;
                case 3:
                    if (dst_idx & 1) {
                        cpu_cmd_dcx(cpu, dst_idx>>1);
                    }
                    else {
                        cpu_cmd_inx(cpu, dst_idx>>1);
                    }
                    break;
                case 4:
                     cpu_cmd_inr(cpu, dst_idx);
                    break;
                case 5:
                     cpu_cmd_dcr(cpu, dst_idx);
                    break;
                case 6:
                    cpu_cmd_mvi(cpu, dst_idx);
                    break;
                case 7:
                    switch (dst_idx) {
                        case 0:
                            cpu_cmd_rlc(cpu);
                            break;
                        case 1:
                            cpu_cmd_rrc(cpu);
                            break;
                        case 2:
                            cpu_cmd_ral(cpu);
                            break;
                        case 3:
                            cpu_cmd_rar(cpu);
                            break;
                        case 4:
                            cpu_cmd_daa(cpu);
                            break;
                        case 5:
                            cpu_cmd_cma(cpu);
                            break;
                        case 6:
                            cpu_cmd_stc(cpu);
                            break;
                        case 7:
                            cpu_cmd_cmc(cpu);
                            break;
                    }
                    break;
            }
            break;
        }
        case 0x40:
            cpu_cmd_mov(cpu, dst_idx, src_idx);
            break;
        case 0x80:
            cpu_cmd_alu(cpu, dst_idx, src_idx);
            break;
        case 0xc0: {
            switch (src_idx) {
                case 0:
                    cpu_cmd_r(cpu, dst_idx);
                    break;
                case 1:
                    if (dst_idx & 1) {
                        switch(dst_idx>>1) {
                            case 0:
                                cpu_cmd_ret(cpu);
                                break;
                            case 1:
#ifdef CPU_MNEMONIC_ENABLE
                                cpu->invalid_op = 1;
#endif
                                break;
                            case 2:
                                cpu_cmd_pchl(cpu);
                                break;
                            case 3:
                                cpu_cmd_sphl(cpu);
                                break;
                        }
                    }
                    else {
                        cpu_cmd_pop(cpu, dst_idx>>1);
                    }
                    break;
                case 2:
                    cpu_cmd_j(cpu, dst_idx);
                    break;
                case 3:
                    switch (dst_idx) {
                        case 0:
                            cpu_cmd_jmp(cpu);
                            break;
                        case 1:
#ifdef CPU_MNEMONIC_ENABLE
                            cpu->invalid_op = 1;
#endif
                            break;
                        case 2:
                            cpu_cmd_out(cpu);
                            break;
                        case 3:
                            cpu_cmd_in(cpu);
                            break;
                        case 4:
                            cpu_cmd_xthl(cpu);
                            break;
                        case 5:
                            cpu_cmd_xchg(cpu);
                            break;
                        case 6:
                            cpu_cmd_di(cpu);
                            break;
                        case 7:
                            cpu_cmd_ei(cpu);
                            break;
                    }
                    break;
                case 4:
                    cpu_cmd_c(cpu, dst_idx);
                    break;
                case 5:
                    if (dst_idx & 1) {
                        if (dst_idx == 1) {
                            cpu_cmd_call(cpu);
                        }
#ifdef CPU_MNEMONIC_ENABLE
                        else {
                            cpu->invalid_op = 1;
                        }
#endif
                    }
                    else {
                        cpu_cmd_push(cpu, dst_idx>>1);
                    }
                    break;
                case 6:
                    cpu_cmd_i(cpu, dst_idx);
                    break;
                case 7:
                    cpu_cmd_rst(cpu, dst_idx);
                    break;
            }
            break;
        }
    }
#ifdef CPU_CYCLES_ENABLE
    uint32_t us_timer = cpu_time();
    if (us_timer < cpu->us_timer) { //65536 us
        cpu->steps += 1;
        if (cpu->steps >= 100) {
            cpu->speed = cpu->cycles >> 16;
            cpu->cycles = 0;
            ESP_LOGI(TAG, "speed: %d.%02dMHz", cpu->speed/100, cpu->speed%100);
            cpu->steps = 0;
        }
    }
    cpu->us_timer = us_timer;
#endif
#ifdef CPU_MNEMONIC_ENABLE
    cpu_trace(cpu);
#endif

    return ESP_OK;
}

