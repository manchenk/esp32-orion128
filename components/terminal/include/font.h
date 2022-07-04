/*
 * This file is part of the terminal component distribution
 * (https://gitlab.romanchenko.su/esp/components/terminal.git).
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
#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>
#include "display.h"
#include "esp_err.h"

typedef enum font_type {
    FONT_TYPE_I1 = 0,
    FONT_TYPE_C16,
    FONT_TYPE_MAX
} font_type_t;

typedef struct font {
    int width;
    int height;
    int is_mirror;
    font_type_t type;
    uint8_t *xlat;
    const uint8_t *data;
} font_t;

esp_err_t font_create(font_t **pfont);
esp_err_t font_init(font_t *font);
esp_err_t font_done(font_t *font);
const uint8_t *font_get_data_ptr(font_t *font, uint8_t index, display_point_t *p);


#endif // __FONT_H__
