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
#include <string.h>
#include "screen.h"

#include "debug.h"

static const char __attribute__((unused)) *TAG = "screen";

screen_symbol_t screen_get_symbol(screen_t *screen, screen_rect_t *r, screen_point_t *p) {
    r = r;
    return screen->buffer[p->y*screen->width + p->x];
}

void screen_draw_window(screen_t *screen, screen_rect_t *r, screen_draw_func_t draw) {
    screen_point_t p;
    for (p.y = r->top; p.y < r->top + r->height; ++p.y) {
        for (p.x = r->left; p.x < r->left + r->width; ++p.x) {
            screen_symbol_t s = draw(screen, r, &p);
            screen_out_symbol(screen, &p, &s);
        }
    }
}

esp_err_t screen_create(screen_t **pscreen)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(pscreen ? ESP_OK : ESP_ERR_INVALID_ARG);

    screen_t *scr = (screen_t *)malloc(sizeof(screen_t));
    ESP_ERROR_CHECK(scr ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(scr, sizeof(screen_t));

    *pscreen = scr;

    return r;
}


static void screen_init(screen_t *screen)
{
    int i;
    size_t size = screen->width * screen->height;
    screen->buffer = (screen_symbol_t *)malloc(size * sizeof(screen_symbol_t));
    ESP_ERROR_CHECK(screen->buffer ? ESP_OK : ESP_ERR_NO_MEM);
    screen_symbol_t *ptr = screen->buffer;
    for (i = 0; i < size; ++i) (ptr++)->item = screen->default_symbol.item;
}

esp_err_t screen_init_bitmap(screen_t *screen, display_bitmap_t *canvas, font_t *font)
{
    esp_err_t r = ESP_OK;

    ESP_ERROR_CHECK(screen ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(canvas ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(font ? ESP_OK : ESP_ERR_INVALID_ARG);

    screen->default_symbol.symb = 0x20;
    screen->default_symbol.front = SCREEN_COLOR_WHITE;
    screen->default_symbol.back = SCREEN_COLOR_BLACK;
    screen->canvas = canvas;
    screen->display = NULL;
    screen->font = font;
    screen->width = canvas->bounds.width / font->width;
    screen->height = canvas->bounds.height / font->height;

    screen_init(screen);

    return r;
}

esp_err_t screen_init_display(screen_t *screen, display_t *display, font_t *font)
{
    esp_err_t r = ESP_OK;

    ESP_ERROR_CHECK(screen ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(font ? ESP_OK : ESP_ERR_INVALID_ARG);

    screen->default_symbol.symb = 0x20;
    screen->default_symbol.front = SCREEN_COLOR_WHITE;
    screen->default_symbol.back = SCREEN_COLOR_BLACK;
    screen->canvas = NULL;
    screen->display = display;
    screen->font = font;
    screen->width = display->bounds.width / font->width;
    screen->height = display->bounds.height / font->height;

    screen_init(screen);

    return r;
}

esp_err_t screen_done(screen_t *screen)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(screen ? ESP_OK : ESP_ERR_INVALID_ARG);

    if (screen->buffer)
        free(screen->buffer);

    free(screen);

    return r;
}

typedef struct screen_symbol_arg {
    screen_symbol_t *symbol;
    font_t *font;
} screen_symbol_arg_t;

static int screen_get_symbol_color_8i4(const display_point_t *p, const display_refresh_info_t *win_info, void *color)
{
    int res = 0;
    screen_symbol_arg_t *args = (screen_symbol_arg_t *)win_info->color.args;
    font_t *font = args->font;
    const display_rectangle_t *r = &win_info->rectangle;
    if (p->x >= r->left && p->x < r->left + r->width && p->y >= r->top && p->y < r->top + r->height) {
        display_point_t ps = {
            x: p->x - r->left,
            y: p->y - r->top
        };
        if (font->is_mirror)
            ps.x = r->width - 1 - ps.x;
        uint8_t data = *font_get_data_ptr(args->font, args->symbol->symb, &ps);
        uint32_t out_data = 0;
        size_t bit;
        for (bit = 0; bit < 8; ++bit) {
            out_data <<= 4;
            if (font->is_mirror) {
                if (data & 1)
                    out_data |= args->symbol->front;
                else
                    out_data |= args->symbol->back;
                data >>= 1;
            }
            else {
                if (data & 0x80)
                    out_data |= args->symbol->front;
                else
                    out_data |= args->symbol->back;
                data <<= 1;
            }
        }
        *(uint32_t *)color = out_data;
        res = 1;
    }
    return res;
}

static int screen_get_symbol_color_bw1(const display_point_t *p, const display_refresh_info_t *win_info, void *color)
{
    int res = 0;
    screen_symbol_arg_t *args = (screen_symbol_arg_t *)win_info->color.args;
    font_t *font = args->font;
    const display_rectangle_t *r = &win_info->rectangle;
    if (p->x >= r->left && p->x < r->left + r->width && p->y >= r->top && p->y < r->top + r->height) {
        display_point_t ps = {
            x: p->x - r->left,
            y: p->y - r->top
        };
        if (font->is_mirror)
            ps.x = r->width - 1 - ps.x;

        uint8_t data = *font_get_data_ptr(args->font, args->symbol->symb, &ps);
        *(uint32_t *)color = (data & (1 << (ps.x & 7))) ? 0 : 1;
        res = 1;
    }
    else {
        ESP_LOGI(TAG, "screen_get_symbol_color_bw1 p->x: %d, p->y: %d", p->x, p->y);
        ESP_LOGI(TAG, "screen_get_symbol_color_bw1 l:%d, t:%d, w:%d, h:%d", r->left, r->top, r->width, r->height);
    }
    return res;
}

static int screen_get_symbol_color_c16(const display_point_t *p, const display_refresh_info_t *win_info, void *color)
{
    int res = 0;
    screen_symbol_arg_t *args = (screen_symbol_arg_t *)win_info->color.args;
    font_t *font = args->font;
    const display_rectangle_t *r = &win_info->rectangle;
    if (p->x >= r->left && p->x < r->left + r->width && p->y >= r->top && p->y < r->top + r->height) {
        display_point_t dp = {
            x: p->x - r->left,
            y: p->y - r->top
        };
        if (font->is_mirror)
            dp.x = r->width - 1 - dp.x;
        *(uint8_t **)color = (uint8_t *)font_get_data_ptr(args->font, args->symbol->symb, &dp);
        res = 1;
    }
    return res;
}

void screen_out_sprite(screen_t *screen, display_point_t *p, screen_symbol_t *symbol)
{
    display_color_callback_t get_color = NULL;
    display_color_format_t format = DISPLAY_COLOR_UNKNOWN;
    device_color_format_t device_format = DEVICE_COLOR_UNKNOWN;
    int ok = 1;

    if (screen->font->type == FONT_TYPE_I1) {
        int bpp = 0;
        const display_t *display = screen->display;
        if (!display) {
           if (screen->canvas)
               display = screen->canvas->display;
        }
        if (display)
            bpp = display->hardware->bpp;
        if (bpp < 8) {
            get_color = screen_get_symbol_color_bw1;
            format = DISPLAY_COLOR_BW1;
            device_format = DEVICE_COLOR_BW1LE;
        }
        else {
            get_color = screen_get_symbol_color_8i4;
            format = DISPLAY_COLOR_8I4;
            device_format = DEVICE_COLOR_RGB555;
        }
    }
    else if (screen->font->type == FONT_TYPE_C16) {
        get_color = screen_get_symbol_color_c16;
        format = DISPLAY_COLOR_PRGB555;
        device_format = DEVICE_COLOR_RGB555;
    }
    else
        ok = 0;

    if (ok) {
        screen_symbol_arg_t args = {
            symbol: symbol,
            font: screen->font,
        };
        if (screen->canvas) {
            display_refresh_info_t win_info = {
                rectangle: {
                    left:   p->x,
                    top:    p->y,
                    width:  screen->font->width,
                    height: screen->font->height
                },
                bitmap: screen->canvas,
                color: {
                    format: format,
                    args: &args,
                    get: get_color
                }
            };
            ESP_ERROR_CHECK(display_bitmap_refresh(&win_info));
        }
        else {
            display_bitmap_t *canvas;
            ESP_ERROR_CHECK(display_bitmap_create(&canvas));
            display_rectangle_t *b = &canvas->bounds;
            b->left = p->x;
            b->top = p->y;
            b->width = screen->font->width;
            b->height = screen->font->height;
            canvas->format = device_format;
            //ESP_LOGI(TAG, "screen_out_sprite l:%d, t:%d, w:%d, h:%d", b->left, b->top, b->width, b->height);
            //ESP_LOGI(TAG, "screen_out_sprite get:%p, format:%d, dev_formatw:%d", get_color, format, device_format);
            ESP_ERROR_CHECK(display_bitmap_init(canvas, screen->display));
            display_refresh_info_t win_info = {
                rectangle: {
                    left:   0,
                    top:    0,
                    width:  b->width,
                    height: b->height
                },
                bitmap: canvas,
                color: {
                    format: format,
                    args: &args,
                    get: get_color
                }
            };
            ESP_ERROR_CHECK(display_bitmap_refresh(&win_info));
            ESP_ERROR_CHECK(display_refresh(canvas));
            ESP_ERROR_CHECK(display_bitmap_done(canvas));
        }
    }
}

void screen_out_symbol(screen_t *screen, screen_point_t *p, screen_symbol_t *s) {
    display_point_t dp = {
        x: p->x * screen->font->width,
        y: p->y * screen->font->height
    };
    screen_symbol_t *symbol = &screen->buffer[p->y*screen->width + p->x];
    symbol->item = s->item;
    screen_out_sprite(screen, &dp, symbol);
}

void screen_scroll(screen_t *screen) {
    screen_point_t p;
    screen_symbol_t *dst = screen->buffer;
    screen_symbol_t *src = dst + screen->width;
    for (p.y = 0; p.y < screen->height-1; ++p.y) {
        for (p.x = 0; p.x < screen->width; ++p.x) {
            *dst = *src++;
            screen_out_symbol(screen, &p, dst);
            ++dst;
        }
    }
    dst = &screen->buffer[screen->width*(screen->height-1)];
    p.y = screen->height-1;
    for (p.x = 0; p.x < screen->width; ++p.x) {
        dst->item = screen->default_symbol.item;
        screen_out_symbol(screen, &p, dst);
        ++dst;
    }
}
