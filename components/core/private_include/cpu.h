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
 */#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

#include "sdkconfig.h"
#include "esp_err.h"

#ifdef CONFIG_CPU_MNEMONIC_ENABLE
#define CPU_MNEMONIC_ENABLE
#endif

#ifdef CONFIG_CPU_CYCLES_ENABLE
#define CPU_CYCLES_ENABLE
#endif

typedef const uint8_t * (*cpu_rd_pointer_cb_t)(uint16_t addr, void *arg);
typedef uint8_t * (*cpu_wr_pointer_cb_t)(uint16_t addr, void *arg);

typedef struct cpu {
    uint16_t pc;
    uint16_t sp;
    uint8_t is_word;
    uint8_t cmd;
#ifdef CPU_CYCLES_ENABLE
    uint32_t cycles;
    uint32_t us_timer;
    uint32_t speed;
    uint32_t steps;
#endif
#ifdef CPU_MNEMONIC_ENABLE
    uint16_t save_pc;
    uint8_t invalid_op;
#endif

    // CPU_REG_FILE_SIZE = 8
    uint8_t *reg_file;
    // CPU_PAIR_SIZE = 4
    uint16_t **pair;
    // CPU_PAIR_SIZE = 4
    uint16_t **push_pop_pair;
    // CPU_REG_SIZE = 8
    uint8_t **reg;

    cpu_rd_pointer_cb_t reader;
    cpu_wr_pointer_cb_t writer;
    void *memory;

} cpu_t;

esp_err_t cpu_create(cpu_t **pcpu);
esp_err_t cpu_init(cpu_t *cpu);
esp_err_t cpu_done(cpu_t *cpu);
esp_err_t cpu_reset(cpu_t *cpu);
esp_err_t cpu_step(cpu_t *cpu);

#endif // __CPU_H__
