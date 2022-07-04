/*
 * This file is part of the display interface distribution
 * (https://gitlab.romanchenko.su/esp/components/display.git).
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
#include "colors.h"
#include "display_test.h"

 #define ST7796S_TEST_MODE 0

int display_test_draw(const display_point_t *p, const display_refresh_info_t *info, void *color)
{
    ESP_ERROR_CHECK(info ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(info->color.args ? ESP_OK : ESP_ERR_INVALID_STATE);
    int count = *(int *)info->color.args;
    const display_bitmap_t *bitmap = info->bitmap;
    const display_rectangle_t *b = &bitmap->bounds;
#if ST7796S_TEST_MODE == 0
    display_color_hsv_t hsv;
    display_color_rgb888_t rgb24;
    display_color_rgb555_t rgb16;

    display_point_t center = {
        x: b->width / 2,
        y: b->height / 2
    };
    uint32_t r = (3 * b->height) / 8;
    int32_t val = (5*p->x/7 - p->y - r)*((p->x - center.x)*(p->x - center.x) + (p->y - center.y)*(p->y - center.y) - r*r) / r / r ;
    hsv.h = (val+(count<<4)) % 0x600;
    hsv.s = 0xff;
    hsv.v = 0xff;
    display_hsv_rgb888(&rgb24, &hsv);
    display_rgb888_rgb555(&rgb16, &rgb24);
    rgb16.brightness = 1;
    *(uint16_t *)color = display_rgb555_uint16(&rgb16);
    return 1;
#elif ST7796S_TEST_MODE == 1
    display_color_rgb555_t tmp;
    uint32_t width = b->width;
    uint16_t val = ((p->x + (count << 2)) << 5)/width;
    uint8_t br = (val & 0x20) ? 1 : 0;
    tmp.r = (val &  0x40) ? (val & 0x1f) ^ (br ? 0x1f : 0) : 0;
    tmp.g = (val &  0x80) ? (val & 0x1f) ^ (br ? 0x1f : 0) : 0;
    tmp.b = (val & 0x100) ? (val & 0x1f) ^ (br ? 0x1f : 0) : 0;
    tmp.brightness = br;
    *(uint16_t *)color = display_rgb555_uint16(&tmp);
    return 1;
#elif ST7796S_TEST_MODE == 2
    display_color_rgb888_t rgb24;
    display_color_rgb555_t rgb16;
    uint32_t width = b->width;
    uint16_t val = ((p->x + (count << 2)) << 8)/width;
    uint8_t br = (val & 0x100) ? 1 : 0;
    rgb24.r = (val & 0x200) ? (val & 0xff) ^ (br ? 0xff : 0) : 0;
    rgb24.g = (val & 0x400) ? (val & 0xff) ^ (br ? 0xff : 0) : 0;
    rgb24.b = (val & 0x800) ? (val & 0xff) ^ (br ? 0xff : 0) : 0;
    display_rgb888_rgb555(&rgb16, &rgb24);
    rgb16.brightness = br;
    *(uint16_t *)color = display_rgb555_uint16(&rgb16);
    return 1;
#elif ST7796S_TEST_MODE == 3
    display_color_hsv_t hsv;
    display_color_rgb888_t rgb24;
    display_color_rgb555_t rgb16;
    uint32_t width = b->width;
    hsv.h = (0x600*p->x/width+count) % 0x600;
    hsv.s = 0xff;
    hsv.v = 0xff;
    display_hsv_rgb888(&rgb24, &hsv);
    display_rgb888_rgb555(&rgb16, &rgb24);
    rgb16.brightness = 1;
    *(uint16_t *)color = display_rgb555_uint16(&rgb16);
    return 1;
#elif ST7796S_TEST_MODE == 4
    if (p->x == 0 || p->y == 0 || (p->x == p->y && p->x < 25))
        *(uint16_t *)color = 0xffff;
    else
        *(uint16_t *)color = 0;
    return 1;

#endif
}


