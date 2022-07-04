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
#include <stdint.h>
#include <string.h>

#include "console.h"
#include "debug.h"

static const char __attribute__((unused)) *TAG = "console";

esp_err_t console_create(console_t **pcon)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(pcon ? ESP_OK: ESP_ERR_INVALID_ARG);

    console_t *con = (console_t *)malloc(sizeof(console_t));
    ESP_ERROR_CHECK(con ? ESP_OK: ESP_ERR_NO_MEM);

    bzero(con, sizeof(console_t));

    *pcon = con;
    return r;
}


esp_err_t console_init(console_t *con, screen_t *scr)
{
    esp_err_t r = ESP_OK;

    ESP_ERROR_CHECK(con ? ESP_OK: ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(scr ? ESP_OK: ESP_ERR_INVALID_ARG);

    con->cursor.x = 0;
    con->cursor.y = 0;
    con->screen = scr;
    return r;
}

esp_err_t console_done(console_t *con)
{
    esp_err_t r = ESP_OK;

    ESP_ERROR_CHECK(con ? ESP_OK: ESP_ERR_INVALID_ARG);
    free(con);

    return r;
}

static void console_next_line(console_t *con) {
    if (con->cursor.y == con->screen->height-1) {
        screen_scroll(con->screen);
    }
    else {
        con->cursor.y++;
    }
    con->cursor.x = 0;
}

static inline void console_cur_left(console_t *con) {
    if (con->cursor.x > 0) {
        --con->cursor.x;
    }
}
static inline void console_cur_right(console_t *con) {
    if (con->cursor.x < con->screen->width-1) {
        ++con->cursor.x;
    }
}
static inline void console_cur_up(console_t *con) {
    if (con->cursor.y > 0) {
        --con->cursor.y;
    }
}
static inline void console_cur_down(console_t *con) {
    if (con->cursor.y < con->screen->height-1) {
        ++con->cursor.y;
    }
}
static inline void console_cur_tab(console_t *con) {
    con->cursor.x &= ~3;
    con->cursor.x += 4;
    if (con->cursor.x >= con->screen->width) {
        console_next_line(con);
    }
}

static inline void console_cur_home(console_t *con) {
    con->cursor.x = 0;
    con->cursor.y = 0;
}

static inline void console_cur_clean(console_t *con) {
}

static inline void console_cur_set(console_t *con, uint32_t data) {
    uint8_t x = (data>>8) & 0xff;
    uint8_t y = data & 0xff;
    x -= 0x20;
    y -= 0x20;
    x %= con->screen->width;
    y %= con->screen->height;
    con->cursor.x = x;
    con->cursor.y = y;
}

static inline void console_color_set(console_t *con, uint32_t data) {
    uint8_t b = (data>>8) & 0xff;
    uint8_t f = data & 0xff;
    b -= 0x20;
    f -= 0x20;
    b &= 0x0f;
    f &= 0x0f;
    con->screen->default_symbol.front = f;
    con->screen->default_symbol.back = b;
}

static void console_out_char(console_t *con, char c) {
    static uint32_t esc_serial = 0;
    static uint8_t state = 0;
    if (esc_serial) {
        esc_serial <<= 8;
        esc_serial |= c;
        switch(esc_serial) {
            case 0x1b41:
                console_cur_up(con);
                break;
            case 0x1b42:
                console_cur_down(con);
                break;
            case 0x1b43:
                console_cur_right(con);
                break;
            case 0x1b44:
                console_cur_left(con);
                break;
            case 0x1b45:
                console_cur_clean(con);
                break;
            case 0x1b59:
                state = 1;
                return;
            case 0x1b5a:
                state = 2;
                return;
            default:
                if (state) {
                    if ((esc_serial & 0xff000000) == 0x1b000000) {
                        switch(state) {
                            case 1:
                                console_cur_set(con, esc_serial & 0xffff);
                                break;
                            case 2:
                                console_color_set(con, esc_serial & 0xffff);
                                break;
                        }
                        break;
                    }
                    else return;
                }
                else break;
        }
        state = 0;
        esc_serial = 0;
    }
    else {
        switch (c) {
            case CONSOLE_CHAR_LEFT:
                console_cur_left(con);
                break;
            case CONSOLE_CHAR_TAB:
                console_cur_tab(con);
                break;
            case CONSOLE_CHAR_LF:
                console_next_line(con);
                break;
            case CONSOLE_CHAR_HOME:
                console_cur_home(con);
                break;
            case CONSOLE_CHAR_CR:
                con->cursor.x = 0;
                break;
            case CONSOLE_CHAR_RIGHT:
                console_cur_right(con);
                break;
            case CONSOLE_CHAR_UP:
                console_cur_up(con);
                break;
            case CONSOLE_CHAR_DOWN:
                console_cur_down(con);
                break;
            case CONSOLE_CHAR_ESC:
                esc_serial = 0x1b;
                break;
            case CONSOLE_CHAR_CLEAN:
                console_cur_clean(con);
                break;
            default: {
                screen_symbol_t s = {symb: c, front: con->screen->default_symbol.front, back: con->screen->default_symbol.back};
                screen_out_symbol(con->screen, &con->cursor, &s);
                ++con->cursor.x;
                if (con->cursor.x >= con->screen->width) {
                    console_next_line(con);
                }
                break;
            }
        }
    }
}

void console_out_string(console_t *con, const char *str) {
    const char *ptr = str;
    while(*ptr) {
        console_out_char(con, *ptr++);
    }
}
 

