/*
 * This file is part of the esp32 components distribution
 * (https://gitlab.romanchenko.su/esp/components/bus.git).
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
 */
#ifndef __BUS_H__
#define __BUS_H__

#include <inttypes.h>

#include "esp_err.h"

typedef enum {
    BUS_WRITE = 0,
    BUS_READ = 1,
    BUS_WRITE_READ = 2
} bus_operation_t;

typedef enum {
    BUS_BEGIN = 0,
    BUS_COMMAND = 1,
    BUS_DATA = 2,
    BUS_END = 3
} bus_state_t;

typedef struct bus_transaction {
    uint32_t address;
    bus_operation_t operation;
    bus_state_t state;
    size_t length;
    const void *out_data;
    void *in_data;
} bus_transaction_t;

struct bus;

typedef esp_err_t (*bus_get_config_t)(struct bus *bus);
typedef esp_err_t (*bus_init_t)(struct bus *bus);
typedef esp_err_t (*bus_done_t)(struct bus *bus);
typedef esp_err_t (*bus_start_t)(struct bus *bus, bus_transaction_t *transactions, size_t trans_num);

typedef struct bus {
    void *device;
    bus_get_config_t get_config;
    bus_init_t init;
    bus_done_t done;
    bus_start_t start;
} bus_t;


#endif // __BUS_H__
