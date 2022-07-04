/*
 * This file is part of the parallel bus driver distribution
 * (https://gitlab.romanchenko.su/esp/components/parbus.git).
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
#ifndef __PARBUS_H__
#define __PARBUS_H__

#include "sdkconfig.h"
#include "bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

typedef struct par_bus_command {
    uint8_t command;
    uint32_t length;
    uint8_t *data;
} par_bus_command_t;

typedef struct parbus_config {
    gpio_num_t rd;
    gpio_num_t wr;
    gpio_num_t rs;
    gpio_num_t cs;
    gpio_num_t d0;
} parbus_config_t;

typedef struct parbus {
    parbus_config_t config;
    uint32_t mask;
    uint32_t mask_rd;
    uint32_t mask_wr;
    uint32_t mask_rs;
    uint32_t mask_cs;
} parbus_t;

esp_err_t parbus_create(bus_t **pbus);

#endif // __PARBUS_H__
