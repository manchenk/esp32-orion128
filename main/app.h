/*
 * This file is part of the esp32-orion128 distribution
 * (https://gitlab.romanchenko.su/esp/esp32/esp32-orion128.git).
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
#ifndef __APP_H__
#define __APP_H__

//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

#include "bus.h"
#include "display.h"
#include "screen.h"
#include "console.h"
#include "font.h"
#include "computer.h"

typedef struct app {
    bus_t *bus;
    display_t *lcd;
    screen_t *screen;
    font_t *font;
    console_t *cout;
    computer_t *computer;
} app_t;

esp_err_t app_create(app_t **papp);
esp_err_t app_init(app_t *app);
esp_err_t app_done(app_t *app);
esp_err_t app_run(app_t *app);

#endif // __APP_H__

