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
 */#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "memory.h"

#define KEYBOARD_FIELDS_NUM 8

typedef struct keyboard {
    uint8_t fields[KEYBOARD_FIELDS_NUM];
    uint8_t flags;
    uint32_t count;
    uint8_t tracing;
    QueueHandle_t queue;
    TaskHandle_t task;
} keyboard_t;


esp_err_t keyboard_create(keyboard_t **pkbd);
esp_err_t keyboard_init(keyboard_t *kbd);
esp_err_t keyboard_step(keyboard_t *kbd, memory_t *mem);
esp_err_t keyboard_done(keyboard_t *kbd);

#endif // __KEYBOARD_H__
