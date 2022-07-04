/*
 * This file is part of the st7796s display driver distribution
 * (https://gitlab.romanchenko.su/esp/components/st7796s.git).
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
#include "st7796s.h"
#include "esp_log.h"
#include "esp32/rom/ets_sys.h"
#include "debug.h"
#include "st7796s_cmd.h"

static const char __attribute__((unused)) *TAG = "st7796s";

#define ST7796S_PALETTE_SIZE 16
static display_color_rgb555_t st7796s_default_palette[ST7796S_PALETTE_SIZE] = {
    { rgb: ST7796S_COLOR_BLACK },
    { rgb: ST7796S_COLOR_D_BLUE },
    { rgb: ST7796S_COLOR_D_GREEN },
    { rgb: ST7796S_COLOR_D_BLUE | ST7796S_COLOR_D_GREEN },
    { rgb: ST7796S_COLOR_D_RED },
    { rgb: ST7796S_COLOR_D_RED | ST7796S_COLOR_D_BLUE },
    { rgb: ST7796S_COLOR_D_RED | ST7796S_COLOR_D_GREEN },
    { rgb: ST7796S_COLOR_D_RED | ST7796S_COLOR_D_GREEN | ST7796S_COLOR_D_BLUE },
    { rgb: ST7796S_COLOR_BLACK },
    { rgb: ST7796S_COLOR_L_BLUE },
    { rgb: ST7796S_COLOR_L_GREEN },
    { rgb: ST7796S_COLOR_L_BLUE | ST7796S_COLOR_L_GREEN },
    { rgb: ST7796S_COLOR_L_RED },
    { rgb: ST7796S_COLOR_L_RED | ST7796S_COLOR_L_BLUE },
    { rgb: ST7796S_COLOR_L_RED | ST7796S_COLOR_L_GREEN },
    { rgb: ST7796S_COLOR_L_RED | ST7796S_COLOR_L_GREEN | ST7796S_COLOR_L_BLUE }
};

static st7796s_config_t st7796s_default_config = {
    view: {
        left: 0,
        top: 0,
        width: ST7796S_DEFAULT_WIDTH,
        height: ST7796S_DEFAULT_HEIGHT
    },
    hardware: {
        bitmap_extra_size: 0,
        bpp: 16,
        default_format: DEVICE_COLOR_RGB555,
        palette: st7796s_default_palette,
        palette_count: ST7796S_PALETTE_SIZE,
        type: ST7796S_TYPE_480x320
    },
    width: ST7796S_DEFAULT_WIDTH,
    height: ST7796S_DEFAULT_HEIGHT,
    bus: 0,
    rst_io_num: GPIO_NUM_NC,
    backlight_io_num: GPIO_NUM_NC,
    column_offset: 0,
    page_offset: 0,

    command_f0_0: {
        data00: 0xc3
    },
    command_f0_1: {
        data00: 0x96
    },
    command_madctl: {
        dontcare: 0,
        mh: 0,
        bgr: 0,
        ml: 0,
        mv: 1,
        mx: 0,
        my: 0
//        data00: 0x68
    },
    command_3a: {
        data00: 0x05
    },
    command_b0: {
        data00: 0x80
    },
    command_b6: {
        data00: 0x20,
        data01: 0x02
    },
    command_b5: {
        data00: 0x02,
        data01: 0x03,
        data02: 0x00,
        data03: 0x04
    },
    command_b1: {
        data00: 0x80,
        data01: 0x10
    },
    command_b4: {
        data00: 0x00
    },
    command_b7: {
        data00: 0xc6
    },
    command_c5: {
        data00: 0x24
    },
    command_e4: {
        data00: 0x31
    },
    command_e8: {
        data00: 0x40,
        data01: 0x8a,
        data02: 0x00,
        data03: 0x00,
        data04: 0x29,
        data05: 0x19,
        data06: 0xa5,
        data07: 0x33
    },
    command_e0: {
        data00: 0xf0,
        data01: 0x09,
        data02: 0x13,
        data03: 0x12,
        data04: 0x12,
        data05: 0x2b,
        data06: 0x3c,
        data07: 0x44,
        data08: 0x4b,
        data09: 0x1b,
        data0a: 0x18,
        data0b: 0x17,
        data0c: 0x1d,
        data0d: 0x21
    },
    command_e1: {
        data00: 0xf0,
        data01: 0x09,
        data02: 0x13,
        data03: 0x0c,
        data04: 0x0d,
        data05: 0x27,
        data06: 0x3b,
        data07: 0x44,
        data08: 0x4d,
        data09: 0x0b,
        data0a: 0x17,
        data0b: 0x17,
        data0c: 0x1d,
        data0d: 0x21
    },

    command_f0_2: {
        data00: 0xc3
    },
    command_f0_3: {
        data00: 0x69
    },
};


static esp_err_t st7796s_get_default_config(st7796s_config_t *config, st7796s_type_t type)
{
    memcpy(config, &st7796s_default_config, sizeof(st7796s_config_t));
    return ESP_OK;
}

static esp_err_t st7796s_gpio_init(st7796s_t *dev)
{
    gpio_config_t io_conf;
    st7796s_config_t *cfg = &dev->config;
    ESP_ERROR_CHECK(cfg->rst_io_num == GPIO_NUM_NC ? ESP_ERR_INVALID_STATE : ESP_OK);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 0;
    io_conf.pin_bit_mask |= 1 << cfg->rst_io_num;
    if (cfg->backlight_io_num != GPIO_NUM_NC)
        io_conf.pin_bit_mask |= 1 << cfg->backlight_io_num;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(cfg->rst_io_num, 1));

    if (cfg->backlight_io_num != GPIO_NUM_NC)
        ESP_ERROR_CHECK(gpio_set_level(cfg->backlight_io_num, 1));

    return ESP_OK;
}

static esp_err_t st7796s_reset(const st7796s_t *dev) {
    const st7796s_config_t *config = &dev->config;
    ets_delay_us(10000);
    ESP_ERROR_CHECK(gpio_set_level(config->rst_io_num, 0));
    ets_delay_us(10000);
    ESP_ERROR_CHECK(gpio_set_level(config->rst_io_num, 1));
    ets_delay_us(150000);
    return ESP_OK;
}

static esp_err_t st7796s_bus_out(const st7796s_t *dev, const st7796s_spibus_command_out_t *cmd)
{
    esp_err_t r = ESP_OK;
    size_t trans_index = 0;
    bus_transaction_t trans[4] = {0};

    trans[trans_index].state = BUS_COMMAND;
    trans[trans_index].operation = BUS_WRITE;
    trans[trans_index].out_data = &cmd->command;
    trans[trans_index].in_data = NULL;
    trans[trans_index].length = 1;
    ++trans_index;
    if (cmd->length > 0 && cmd->data) {
        trans[trans_index].state = BUS_DATA;
        trans[trans_index].operation = BUS_WRITE;
        trans[trans_index].out_data = cmd->data;
        trans[trans_index].in_data = NULL;
        trans[trans_index].length = cmd->length;
        ++trans_index;
        trans[trans_index].state = BUS_END;
        ++trans_index;
    }
    trans[trans_index].state = BUS_END;
    ++trans_index;

    const st7796s_config_t *cfg = &dev->config;
    xSemaphoreTake(dev->bus_mutex, portMAX_DELAY);
    ESP_ERROR_CHECK(cfg->bus->start(cfg->bus, trans, trans_index));
    xSemaphoreGive(dev->bus_mutex);
    return r;
}

static esp_err_t __attribute__((unused)) st7796s_bus_in(const st7796s_t *dev, st7796s_spibus_command_in_t *cmd)
{
    esp_err_t r = ESP_OK;
    size_t trans_index = 0;
    bus_transaction_t trans[4] = {0};

    trans[trans_index].state = BUS_COMMAND;
    trans[trans_index].operation = BUS_WRITE;
    trans[trans_index].out_data = &cmd->command;
    trans[trans_index].in_data = NULL;
    trans[trans_index].length = 1;
    ++trans_index;
    if (cmd->length > 0 && cmd->data) {
        trans[trans_index].state = BUS_DATA;
        trans[trans_index].operation = BUS_READ;
        trans[trans_index].out_data = NULL;
        trans[trans_index].in_data = cmd->data;
        trans[trans_index].length = cmd->length;
        ++trans_index;
        trans[trans_index].state = BUS_END;
        ++trans_index;
    }
    trans[trans_index].state = BUS_END;
    ++trans_index;

    const st7796s_config_t *cfg = &dev->config;
    xSemaphoreTake(dev->bus_mutex, portMAX_DELAY);
    ESP_ERROR_CHECK(cfg->bus->start(cfg->bus, trans, trans_index));
    xSemaphoreGive(dev->bus_mutex);
    return r;
}

static esp_err_t st7796s_bus_out_command(const st7796s_t *dev, uint8_t command)
{
    esp_err_t r = ESP_OK;
    st7796s_spibus_command_out_t cmd = {
        command: command,
        length: 0,
        data: NULL
    };
    ESP_ERROR_CHECK(st7796s_bus_out(dev, &cmd));
    return r;
}

// SLPIN (10h): Sleep in 
static esp_err_t __attribute__((unused)) st7796s_command_sleep_in(const st7796s_t *device)
{
    return st7796s_bus_out_command(device, ST7796S_COMMAND_SLPIN);
}

// SLPOUT (11h): Sleep Out
static esp_err_t __attribute__((unused)) st7796s_command_sleep_out(const st7796s_t *device)
{
    return st7796s_bus_out_command(device, ST7796S_COMMAND_SLPOUT);
}

static esp_err_t __attribute__((unused)) st7796s_command_inversion_on(const st7796s_t *device) {
    return st7796s_bus_out_command(device, ST7796S_COMMAND_INVON);
}

static esp_err_t __attribute__((unused)) st7796s_command_inversion_off(const st7796s_t *device) {
    return st7796s_bus_out_command(device, ST7796S_COMMAND_INVOFF);
}

// DISPON (29h): Display On
static esp_err_t st7796s_command_display_on(const st7796s_t *device) {
    return st7796s_bus_out_command(device, ST7796S_COMMAND_DISPON);
}

// DISPOFF (28h): Display Off 
static esp_err_t __attribute__((unused)) st7796s_command_display_off(const st7796s_t *device) {
    return st7796s_bus_out_command(device, ST7796S_COMMAND_DISPOFF);
}

//    st7796s_command_f0_t command_f0;
static esp_err_t st7796s_command_f0(const st7796s_t *device, const st7796s_command_f0_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_F0,
        length: sizeof(st7796s_command_f0_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_madctl_t command_madctl;
static esp_err_t st7796s_command_madctl(const st7796s_t *device, const st7796s_command_madctl_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_36,
        length: sizeof(st7796s_command_madctl_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_3a_t command_3a;
static esp_err_t st7796s_command_3a(const st7796s_t *device, const st7796s_command_3a_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_3A,
        length: sizeof(st7796s_command_3a_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_b0_t command_b0;
static esp_err_t st7796s_command_b0(const st7796s_t *device, const st7796s_command_b0_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_B0,
        length: sizeof(st7796s_command_b0_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_b6_t command_b6;
static esp_err_t st7796s_command_b6(const st7796s_t *device, const st7796s_command_b6_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_B6,
        length: sizeof(st7796s_command_b6_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_b5_t command_b5;
static esp_err_t st7796s_command_b5(const st7796s_t *device, const st7796s_command_b5_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_B5,
        length: sizeof(st7796s_command_b5_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_b1_t command_b1;
static esp_err_t st7796s_command_b1(const st7796s_t *device, const st7796s_command_b1_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_B1,
        length: sizeof(st7796s_command_b1_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_b4_t command_b4;
static esp_err_t st7796s_command_b4(const st7796s_t *device, const st7796s_command_b4_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_B4,
        length: sizeof(st7796s_command_b4_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_b7_t command_b7;
static esp_err_t st7796s_command_b7(const st7796s_t *device, const st7796s_command_b7_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_B7,
        length: sizeof(st7796s_command_b7_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_c5_t command_c5;
static esp_err_t st7796s_command_c5(const st7796s_t *device, const st7796s_command_c5_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_C5,
        length: sizeof(st7796s_command_c5_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_e4_t command_e4;
static esp_err_t st7796s_command_e4(const st7796s_t *device, const st7796s_command_e4_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_E4,
        length: sizeof(st7796s_command_e4_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_e8_t command_e8;
static esp_err_t st7796s_command_e8(const st7796s_t *device, const st7796s_command_e8_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_E8,
        length: sizeof(st7796s_command_e8_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_e0_t command_e0;
static esp_err_t st7796s_command_e0(const st7796s_t *device, const st7796s_command_e0_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_E0,
        length: sizeof(st7796s_command_e0_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

//    st7796s_command_e1_t command_e1;
static esp_err_t st7796s_command_e1(const st7796s_t *device, const st7796s_command_e1_t *data)
{
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_E1,
        length: sizeof(st7796s_command_e1_t),
        data: data
    };
    return st7796s_bus_out(device, &cmd);
}

static esp_err_t __attribute__((unused)) st7796s_command_c2(const st7796s_t *device) {
    return st7796s_bus_out_command(device, ST7796S_COMMAND_C2);
}

static esp_err_t __attribute__((unused)) st7796s_command_a7(const st7796s_t *device) {
    return st7796s_bus_out_command(device, ST7796S_COMMAND_A7);
}

static esp_err_t __attribute__((unused)) st7796s_command_13(const st7796s_t *device) {
    return st7796s_bus_out_command(device, ST7796S_COMMAND_13);
}

// CASET (2Ah): Column Address Set 
static esp_err_t st7796s_column_address_set(const st7796s_t *device, uint16_t beg, uint16_t end) {
    uint8_t buf[4];
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_CASET,
        length: sizeof(buf),
        data: buf
    };
    beg += device->config.column_offset;
    end += device->config.column_offset;
    buf[0] = (beg >> 8) & 0xff;
    buf[1] = beg & 0xff;
    buf[2] = (end >> 8) & 0xff;
    buf[3] = end & 0xff;
    return st7796s_bus_out(device, &cmd);
}

// RASET (2Bh): Row Address Set
static esp_err_t st7796s_page_address_set(const st7796s_t *device, uint16_t beg, uint16_t end) {
    uint8_t buf[4];
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_PASET,
        length: sizeof(buf),
        data: buf
    };
    beg += device->config.page_offset;
    end += device->config.page_offset;
    buf[0] = (beg >> 8) & 0xff;
    buf[1] = beg & 0xff;
    buf[2] = (end >> 8) & 0xff;
    buf[3] = end & 0xff;
    return st7796s_bus_out(device, &cmd);
}

// RAMWR (2Ch): Memory Write 
static esp_err_t st7796s_memory_write(const st7796s_t *device, uint32_t len, const uint8_t *_data) {
    st7796s_spibus_command_out_t cmd = {
        command: ST7796S_COMMAND_RAMWR,
        length: len,
        data: _data
    };
    return st7796s_bus_out(device, &cmd);
}

static esp_err_t st7796s_chip_init(const st7796s_t *device, const st7796s_config_t *config)
{
    esp_err_t r = ESP_OK;

    ESP_ERROR_CHECK(st7796s_reset(device));
    ESP_ERROR_CHECK(st7796s_command_f0(device, &config->command_f0_0));
    ESP_ERROR_CHECK(st7796s_command_f0(device, &config->command_f0_1));
    ESP_ERROR_CHECK(st7796s_command_madctl(device, &config->command_madctl));
    ESP_ERROR_CHECK(st7796s_command_3a(device, &config->command_3a));
    ESP_ERROR_CHECK(st7796s_command_b0(device, &config->command_b0));
    ESP_ERROR_CHECK(st7796s_command_b6(device, &config->command_b6));
    ESP_ERROR_CHECK(st7796s_command_b5(device, &config->command_b5));
    ESP_ERROR_CHECK(st7796s_command_b1(device, &config->command_b1));
    ESP_ERROR_CHECK(st7796s_command_b4(device, &config->command_b4));
    ESP_ERROR_CHECK(st7796s_command_b7(device, &config->command_b7));
    ESP_ERROR_CHECK(st7796s_command_c5(device, &config->command_c5));
    ESP_ERROR_CHECK(st7796s_command_e4(device, &config->command_e4));
    ESP_ERROR_CHECK(st7796s_command_e8(device, &config->command_e8));
    ESP_ERROR_CHECK(st7796s_command_c2(device));
    ESP_ERROR_CHECK(st7796s_command_a7(device));
    ESP_ERROR_CHECK(st7796s_command_e0(device, &config->command_e0));
    ESP_ERROR_CHECK(st7796s_command_e1(device, &config->command_e1));
//    ESP_ERROR_CHECK(st7796s_command_madctl(device, &config->command_madctl_1));
    ESP_ERROR_CHECK(st7796s_command_f0(device, &config->command_f0_2));
    ESP_ERROR_CHECK(st7796s_command_f0(device, &config->command_f0_3));
    ESP_ERROR_CHECK(st7796s_command_13(device));
    ESP_ERROR_CHECK(st7796s_command_sleep_out(device));
    ESP_ERROR_CHECK(st7796s_command_display_on(device));

    return r;
}

static esp_err_t st7796s_bitmap_output(const st7796s_t *device, const display_bitmap_t *bitmap)
{
    esp_err_t err = ESP_OK;
    const st7796s_config_t *config = &device->config;
    const display_rectangle_t *v = &config->view;
    const display_rectangle_t *b = &bitmap->bounds;
    int lt = b->left + v->left;
    int tp = b->top + v->top;
    int rt = lt + b->width - 1;
    int bt = tp + b->height - 1;

//    ESP_LOGI(TAG, "bitmap bounds: l: %3d, t: %3d, w: %3d, h: %3d", b->left, b->top, b->width, b->height);
//    ESP_LOGI(TAG, "device views:  l: %3d, t: %3d, w: %3d, h: %3d", v->left, v->top, v->width, v->height);
//    ESP_LOGI(TAG, "column strart: %3d, end: %3d", lt, rt);
//    ESP_LOGI(TAG, "row    strart: %3d, end: %3d", tp, bt);

    xSemaphoreTake(device->mutex, portMAX_DELAY);
    ESP_ERROR_CHECK(st7796s_column_address_set(device, lt, rt));
    ESP_ERROR_CHECK(st7796s_page_address_set(device, tp, bt));
    ESP_ERROR_CHECK(st7796s_memory_write(device, (b->width * b->height * bitmap->bpp) >> 3, bitmap->data));
    xSemaphoreGive(device->mutex);
    return err;
}

esp_err_t st7796s_init(st7796s_t *device) {
    esp_err_t err = ESP_OK;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_ARG);
    st7796s_config_t *config = &device->config;

    ESP_ERROR_CHECK(st7796s_gpio_init(device));

    device->bus_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(device->bus_mutex == NULL ? ESP_ERR_NO_MEM : ESP_OK);

    device->mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(device->mutex == NULL ? ESP_ERR_NO_MEM : ESP_OK);

    ESP_ERROR_CHECK(st7796s_chip_init(device, config));

    return err;
}

esp_err_t st7796s_done(st7796s_t *device) {
    esp_err_t err = ESP_OK;

    if (device->mutex)
        vSemaphoreDelete(device->mutex);
    if (device->bus_mutex)
        vSemaphoreDelete(device->bus_mutex);

    return err;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t st7796s_display_get_config(const display_t *display, int type)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_ARG);

    st7796s_t *device = (st7796s_t *)display->device;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_STATE);

    ESP_ERROR_CHECK(st7796s_get_default_config(&device->config, type));
    return r;
}


esp_err_t st7796s_display_init(display_t *display, const display_initialization_t *init)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_ARG);
    st7796s_t *device = (st7796s_t *)display->device;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_STATE);

    st7796s_config_t *config = &device->config;

    display_rectangle_t *b = &display->bounds;
    display_rectangle_t *v = &config->view;
    int width, height;
    if (init->orientation == DISPLAY_LANDSCAPE) {
        int t = v->left;
        v->left = v->top;
        v->top = t;
        t = v->width;
        v->width = v->height;
        v->height = t;
        display->orientation = DISPLAY_LANDSCAPE;

        width = config->height;
        height = config->width;
        if (init->flip_vertically) {
            b->left = width - (v->left + v->width);
            b->width = v->width;
        }
        else {
            b->left = v->left;
            b->width = v->width;
        }
        if (init->flip_horizontally) {
            b->top = height - (v->top + v->height);
            b->height = v->height;
        }
        else {
            b->top = v->top;
            b->height = v->height;
        }
    }
    else {
        display->orientation = DISPLAY_PORTRAIT;

        width = config->width;
        height = config->height;
        if (init->flip_vertically) {
            b->left = width - (v->left + v->width);
            b->width = v->width;
        }
        else {
            b->left = v->left;
            b->width = v->width;
        }
        if (init->flip_horizontally) {
            b->top = height - (v->top + v->height);
            b->height = v->height;
        }
        else {
            b->top = v->top;
            b->height = v->height;
        }
    }
    display->hardware = &config->hardware;

    ESP_LOGI(TAG, "display w: %d, h: %d", display->bounds.width, display->bounds.height);

    ESP_ERROR_CHECK(st7796s_init(device));

//    ESP_LOGI(TAG, "display w: %d, h: %d", display->bounds.width, display->bounds.height);

    return r;
}


esp_err_t st7796s_display_done(display_t *display)
{
    esp_err_t r = ESP_OK;
    if (display) {
        st7796s_t *device = (st7796s_t *)display->device;
        ESP_ERROR_CHECK(st7796s_done(device));
        if (display->device)
            free(display->device);
        free(display);
    }
    return r;
}

esp_err_t st7796s_display_refresh(const display_bitmap_t *bitmap)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(bitmap ? ESP_OK : ESP_ERR_INVALID_ARG);
    const display_t *display = bitmap->display;
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_STATE);
    st7796s_t *device = (st7796s_t *)display->device;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(st7796s_bitmap_output(device, bitmap));
    return r;
}

esp_err_t st7796s_create(display_t **pdisplay)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(pdisplay ? ESP_OK : ESP_ERR_INVALID_ARG);

    display_t *display = (display_t *)malloc(sizeof(display_t));
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(display, sizeof(display_t));

    display->device = (st7796s_t *)malloc(sizeof(st7796s_t));
    ESP_ERROR_CHECK(display->device ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(display->device, sizeof(st7796s_t));

    display->background.get = NULL;
    display->background.args = NULL;
    display->get_config = st7796s_display_get_config;
    display->init = st7796s_display_init;
    display->done = st7796s_display_done;
    display->refresh = st7796s_display_refresh;
    *pdisplay = display;
    return r;
}




