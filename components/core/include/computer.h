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
 */
#ifndef __COMPUTER_H__
#define __COMPUTER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "display.h"
#include "memory.h"
#include "cpu.h"
#include "keyboard.h"

typedef struct computer {
    cpu_t *cpu;
    memory_t *mem;
    display_t *display;
    keyboard_t *kbd;
    TaskHandle_t video_task;
    QueueHandle_t video_queue;
} computer_t;

esp_err_t computer_create(computer_t **cmp);
esp_err_t computer_init(computer_t *cmp);
esp_err_t computer_step(computer_t *cmp);
esp_err_t computer_done(computer_t *cmp);


#endif // __COMPUTER_H__
