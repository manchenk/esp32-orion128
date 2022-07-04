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
#include "esp_log.h"
#include "font.h"

static const char __attribute__((unused)) *TAG = "font";

esp_err_t font_create(font_t **pfont)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(pfont ? ESP_OK : ESP_ERR_INVALID_ARG);

    font_t *font = (font_t *)malloc(sizeof(font_t));
    ESP_ERROR_CHECK(font ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(font, sizeof(font_t));

    font->xlat = (uint8_t *)malloc(0x100);
    ESP_ERROR_CHECK(font->xlat ? ESP_OK : ESP_ERR_NO_MEM);

    int i = 0;
    for (i = 0; i < 0x100; ++i)
        font->xlat[i] = i;

    font->is_mirror = false;

    *pfont = font;

    return r;
}

esp_err_t font_init(font_t *font)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(font ? ESP_OK : ESP_ERR_INVALID_ARG);

    ESP_ERROR_CHECK((!font->width || !font->height) ? ESP_ERR_INVALID_STATE : ESP_OK);
    ESP_ERROR_CHECK((font->type >= FONT_TYPE_MAX) ? ESP_ERR_INVALID_STATE : ESP_OK);
    ESP_ERROR_CHECK(font->data ? ESP_OK : ESP_ERR_INVALID_STATE);

    return r;
}

esp_err_t font_done(font_t *font)
{
    esp_err_t r = ESP_OK;

    if (font)
        free(font);

    return r;
}

const uint8_t *font_get_data_ptr(font_t *font, uint8_t index, display_point_t *p)
{
    ESP_ERROR_CHECK(font ? ESP_OK : ESP_ERR_INVALID_ARG);

    int line_size = font->width;
    int ofs  = p->x % font->width;
    int line = p->y % font->height;
    if (font->type == FONT_TYPE_I1) {
        line_size >>= 3;
        ofs >>= 3;
    }
    if (font->type == FONT_TYPE_C16) {
        line_size <<= 1;
        ofs <<= 1;
    }

    //ESP_LOGI(TAG, "char '%c'(%02x) -> '%c'(%02x)", index, index, font->xlat[index], font->xlat[index]);
    index = font->xlat[index];

    return font->data + (index * font->height + line) * line_size + ofs;

}


