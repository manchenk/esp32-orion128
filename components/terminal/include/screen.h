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
#ifndef __SCREEN_H
#define __SCREEN_H

#include <stdint.h>
#include "display.h"
#include "font.h"

#define SCREEN_DEFAULT_ATTRIBUTE 0x0f
#define SCREEN_DEFAULT_CHAR 0x20

typedef struct {
    uint8_t x;
    uint8_t y;
} screen_point_t;

typedef struct {
    uint8_t left;
    uint8_t top;
    uint8_t width;
    uint8_t height;
} screen_rect_t;

typedef enum {
    SCREEN_COLOR_BLACK    = 0,
    SCREEN_COLOR_BLUE     = 1,
    SCREEN_COLOR_GREEN    = 2,
    SCREEN_COLOR_CYAN     = 3,
    SCREEN_COLOR_RED      = 4,
    SCREEN_COLOR_MAGENTA  = 5,
    SCREEN_COLOR_BROWN    = 6,
    SCREEN_COLOR_GRAY     = 7,
    SCREEN_COLOR_LGRAY    = 8,
    SCREEN_COLOR_LBLUE    = 9,
    SCREEN_COLOR_LGREEN   = 10,
    SCREEN_COLOR_LCYAN    = 11,
    SCREEN_COLOR_LRED     = 12,
    SCREEN_COLOR_LMAGENTA = 13,
    SCREEN_COLOR_YELLOW   = 14,
    SCREEN_COLOR_WHITE    = 15
} screen_color_t;


typedef union {
    uint16_t item;
    struct {
        uint8_t symb: 8;
        screen_color_t front: 4; 
        screen_color_t back: 4;
    };
} screen_symbol_t;

struct screen;

typedef screen_symbol_t (*screen_draw_func_t)(struct screen *scr, screen_rect_t *r, screen_point_t *p);

typedef struct screen {
    uint32_t width;
    uint32_t height;
    display_t *display;
    display_bitmap_t *canvas;
    font_t *font;
    screen_symbol_t default_symbol;
    screen_symbol_t *buffer;      //[SCREEN_BUFFER_SIZE];
} screen_t;

esp_err_t screen_create(screen_t **pscreen);
esp_err_t screen_init_canvas(screen_t *screen, display_bitmap_t *canvas, font_t *font);
esp_err_t screen_init_display(screen_t *screen, display_t *display, font_t *font);
esp_err_t screen_init_bitmap(screen_t *screen, display_bitmap_t *canvas, font_t *font);
esp_err_t screen_done(screen_t *screen);

screen_symbol_t screen_get_symbol(screen_t *screen, screen_rect_t *r, screen_point_t *p);
void screen_draw_window(screen_t *screen, screen_rect_t *r, screen_draw_func_t draw);
void screen_out_symbol(screen_t *screen, screen_point_t *p, screen_symbol_t *s);
void screen_out_sprite(screen_t *screen, display_point_t *p, screen_symbol_t *s);
void screen_scroll(screen_t *screen);


#endif // __SCREEN_H
