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
#include "sdkconfig.h"

#include <string.h>
#include "esp_log.h"
#include "app.h"
#include "parbus.h"
#if defined(CONFIG_DISPLAY_TYPE_ILI9486)
#  include "ili9486.h"
#elif defined(CONFIG_DISPLAY_TYPE_ST7796S)
#  include "st7796s.h"
#else
#  error Unknown LCD display type
#endif
#include "computer.h"
#include "console.h"


static const uint8_t app_ram[] asm("_binary_ramdisk1_rom_start");
static const uint8_t app_ram_end[] asm("_binary_ramdisk1_rom_end");

static const uint8_t app_rom[] asm("_binary_monitor2_rom_start");
//static const uint8_t app_rom[] asm("_binary_ram_test_rom_start");
static const uint8_t app_or_dos_rom[]  asm("_binary_romdisk2_rom_start");

static const uint8_t app_font8x8[] asm("_binary_font8x8_fnt_start");
//static const uint8_t app_xlat8x8[] asm("_binary_xlat8x8_bin_start");

static const char __attribute__((unused)) *TAG = "app";


esp_err_t app_create(app_t **papp)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(papp ? ESP_OK : ESP_ERR_INVALID_ARG);
    app_t *app = (app_t *)malloc(sizeof(app_t));
    ESP_ERROR_CHECK(app ? ESP_OK : ESP_ERR_NO_MEM);
    memset(app, 0, sizeof(app_t));

    ESP_LOGI(TAG, "Create application");

    ESP_ERROR_CHECK(parbus_create(&app->bus));
#if defined(CONFIG_DISPLAY_TYPE_ILI9486)
    ESP_ERROR_CHECK(ili9486_create(&app->lcd));
#elif defined(CONFIG_DISPLAY_TYPE_ST7796S)
    ESP_ERROR_CHECK(st7796s_create(&app->lcd));
#endif
    ESP_ERROR_CHECK(font_create(&app->font));
    ESP_ERROR_CHECK(screen_create(&app->screen));
    ESP_ERROR_CHECK(console_create(&app->cout));
    ESP_ERROR_CHECK(computer_create(&app->computer));

    *papp = app;

    return r;
}


static void app_bus_init(app_t *app)
{
    ESP_ERROR_CHECK(app->bus->get_config(app->bus));
    parbus_config_t *config = (parbus_config_t *)app->bus->device;

    config->rd = CONFIG_LCD_RD_PIN;
    config->wr = CONFIG_LCD_WR_PIN;
    config->rs = CONFIG_LCD_RS_PIN;
    config->cs = CONFIG_LCD_CS_PIN;
    config->d0 = CONFIG_LCD_D0_PIN;

    ESP_ERROR_CHECK(app->bus->init(app->bus));
}

static void app_display_init(app_t *app)
{
#if defined(CONFIG_DISPLAY_TYPE_ILI9486)
    ESP_ERROR_CHECK(app->lcd->get_config(app->lcd, ILI9486_TYPE_480x320));
    ili9486_config_t *config = (ili9486_config_t *)app->lcd->device;
#elif defined(CONFIG_DISPLAY_TYPE_ST7796S)
    ESP_ERROR_CHECK(app->lcd->get_config(app->lcd, ST7796S_TYPE_480x320));
    st7796s_config_t *config = (st7796s_config_t *)app->lcd->device;
#endif

    config->rst_io_num = CONFIG_LCD_RST_PIN;
    config->backlight_io_num = GPIO_NUM_NC;
    config->bus = app->bus;
//    config->is_buffered = false;

    display_initialization_t init = {
        orientation: DISPLAY_LANDSCAPE,
        flip_vertically: true,
        flip_horizontally: true
    };
    ESP_ERROR_CHECK(app->lcd->init(app->lcd, &init));
}

static void app_console_init(app_t *app)
{
    font_t *font = app->font;
    font->data = app_font8x8;
//    memcpy(font->xlat, app_xlat8x8, 256);
    font->width = 8;
    font->height = 8;
    font->type = FONT_TYPE_I1;
    font->is_mirror = true;
    ESP_ERROR_CHECK(font_init(font));

    screen_t *screen = app->screen;
    ESP_ERROR_CHECK(screen_init_display(screen, app->lcd, font));

    console_t *cout = app->cout;
    ESP_ERROR_CHECK(console_init(cout, screen));
}

static void app_computer_init(app_t *app)
{
    size_t size = app_ram_end - app_ram + 1;
    if (size > MEMORY_RAM_PAGE1_SIZE)
        size = MEMORY_RAM_PAGE1_SIZE;

    computer_t *computer = app->computer;
    computer->display = app->lcd;
    memory_t *mem = computer->mem;
    memcpy(mem->ram_page1, app_ram, size);
    mem->rom = app_rom;
    mem->rom_disk = app_or_dos_rom;

    computer_init(computer);
}


esp_err_t app_init(app_t *app)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(app ? ESP_OK : ESP_ERR_INVALID_ARG);

    app_bus_init(app);
    app_display_init(app);
    app_console_init(app);
    app_computer_init(app);

    return r;

}

esp_err_t app_done(app_t *app)
{
    ESP_ERROR_CHECK(app ? ESP_OK : ESP_ERR_INVALID_ARG);

    ESP_ERROR_CHECK(console_done(app->cout));
    ESP_ERROR_CHECK(screen_done(app->screen));
    ESP_ERROR_CHECK(font_done(app->font));
    ESP_ERROR_CHECK(app->lcd->done(app->lcd));
    ESP_ERROR_CHECK(app->bus->done(app->bus));

    free(app);

    return ESP_OK;
}

static screen_symbol_t __attribute__((unused)) draw_background(screen_t *scr, screen_rect_t *r, screen_point_t *p) {
    static screen_symbol_t s = {symb: 0xb1, front: SCREEN_COLOR_BLACK, back: SCREEN_COLOR_BLACK};
    return s;
}

static screen_symbol_t __attribute__((unused)) draw_window_border(screen_t *scr, screen_rect_t *r, screen_point_t *p) {
    static screen_symbol_t top_left     = {symb: 0xda, front: SCREEN_COLOR_YELLOW, back: SCREEN_COLOR_LGREEN};
    static screen_symbol_t top_right    = {symb: 0xbf, front: SCREEN_COLOR_YELLOW, back: SCREEN_COLOR_LGREEN};
    static screen_symbol_t bottom_left  = {symb: 0xc0, front: SCREEN_COLOR_YELLOW, back: SCREEN_COLOR_LGREEN};
    static screen_symbol_t bottom_right = {symb: 0xd9, front: SCREEN_COLOR_YELLOW, back: SCREEN_COLOR_LGREEN};
    static screen_symbol_t vertical     = {symb: 0xb3, front: SCREEN_COLOR_YELLOW, back: SCREEN_COLOR_LGREEN};
    static screen_symbol_t horizontal   = {symb: 0xc4, front: SCREEN_COLOR_YELLOW, back: SCREEN_COLOR_LGREEN};
    static screen_symbol_t back         = {symb: 0x20, front: SCREEN_COLOR_YELLOW, back: SCREEN_COLOR_LGREEN};
    if (r->left == p->x && r->top == p->y) return top_left;
    else if (r->left + r->width - 1 == p->x && r->top == p->y) return top_right;
    else if (r->left == p->x && r->top + r->height - 1 == p->y) return bottom_left;
    else if (r->left + r->width - 1 == p->x && r->top + r->height - 1 == p->y) return bottom_right;
    else if (r->left == p->x || r->left + r->width - 1 == p->x) return vertical;
    else if (r->top == p->y || r->top + r->height - 1 == p->y) return horizontal;
    else return back;
}

esp_err_t app_run(app_t *app) {
    uint32_t count = 0;

    display_t *lcd = app->lcd;
    screen_t *scr = app->screen;

    ESP_LOGI(TAG, "LCD size: %d x %d", lcd->bounds.width, lcd->bounds.height);
    ESP_LOGI(TAG, "Screen size: %d x %d", scr->width, scr->height);

    screen_rect_t r = {left: 0, top: 0, width: scr->width, height: scr->height};
    screen_draw_window(scr, &r, draw_background);

    screen_rect_t rw = {left: 18, top: 16, width: scr->width-1-36, height: scr->height-1-32};
    screen_draw_window(scr, &rw, draw_window_border);

    console_t *cout = app->cout;
    console_out_string(cout, "\x1b\x59\x34\x32\x1b\x5a\x21\x2e Start Web server  ");
    console_out_string(cout, "\x1b\x59\x34\x33\x1b\x5a\x2e\x21 Start Orion 128   ");
    console_out_string(cout, "\x1b\x59\x34\x34\x1b\x5a\x21\x2e Start Radio 86 RK ");
    console_out_string(cout, "\x1b\x59\x34\x35\x1b\x5a\x21\x2e Reboot            ");

    while (1) {
        computer_step(app->computer);
        ++count;
    }
    return ESP_OK;
}

