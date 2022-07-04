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
#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "screen.h"

#define CONSOLE_CHAR_LEFT  0x08
#define CONSOLE_CHAR_TAB   0x09
#define CONSOLE_CHAR_LF    0x0a
#define CONSOLE_CHAR_HOME  0x0c
#define CONSOLE_CHAR_CR    0x0d
#define CONSOLE_CHAR_RIGHT 0x18
#define CONSOLE_CHAR_UP    0x19
#define CONSOLE_CHAR_DOWN  0x1a
#define CONSOLE_CHAR_ESC   0x1b
#define CONSOLE_CHAR_CLEAN 0x1f

typedef struct console {
    screen_point_t cursor;
    screen_t *screen;
} console_t;

esp_err_t console_create(console_t **pcon);
esp_err_t console_init(console_t *con, screen_t *screen);
esp_err_t console_done(console_t *con);
void console_out_string(console_t *con, const char *str);


#endif // __CONSOLE_H
