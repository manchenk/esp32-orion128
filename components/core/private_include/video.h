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
 */#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "esp_err.h"
#include "computer.h"

#define VIDEO_DISPLAY_WIDTH 384
#define VIDEO_DISPLAY_HEIGHT 256

typedef enum {
    VIDEO_COLOR_BLACK    = 0,
    VIDEO_COLOR_BLUE     = 1,
    VIDEO_COLOR_GREEN    = 2,
    VIDEO_COLOR_CYAN     = 3,
    VIDEO_COLOR_RED      = 4,
    VIDEO_COLOR_MAGENTA  = 5,
    VIDEO_COLOR_BROWN    = 6,
    VIDEO_COLOR_GRAY     = 7,
    VIDEO_COLOR_LGRAY    = 8,
    VIDEO_COLOR_LBLUE    = 9,
    VIDEO_COLOR_LGREEN   = 10,
    VIDEO_COLOR_LCYAN    = 11,
    VIDEO_COLOR_LRED     = 12,
    VIDEO_COLOR_LMAGENTA = 13,
    VIDEO_COLOR_YELLOW   = 14,
    VIDEO_COLOR_WHITE    = 15
} video_color_t;


esp_err_t video_init(computer_t *cmp);
esp_err_t video_step(computer_t *cmp);

#endif // __VIDEO_H__
