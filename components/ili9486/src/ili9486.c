/*
 * This file is part of the ili9486 display driver distribution
 * (https://gitlab.romanchenko.su/esp/components/ili9486.git).
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
#include "ili9486.h"
#include "esp_log.h"
#include "esp32/rom/ets_sys.h"
#include "debug.h"
#include "ili9486_cmd.h"


static const char __attribute__((unused)) *TAG = "ili9486";

#define ILI9486_PALETTE_SIZE 16
static display_color_rgb555_t ili9486_default_palette[ILI9486_PALETTE_SIZE] = {
    { rgb: ILI9486_COLOR_BLACK },
    { rgb: ILI9486_COLOR_D_BLUE },
    { rgb: ILI9486_COLOR_D_GREEN },
    { rgb: ILI9486_COLOR_D_BLUE | ILI9486_COLOR_D_GREEN },
    { rgb: ILI9486_COLOR_D_RED },
    { rgb: ILI9486_COLOR_D_RED | ILI9486_COLOR_D_BLUE },
    { rgb: ILI9486_COLOR_D_RED | ILI9486_COLOR_D_GREEN },
    { rgb: ILI9486_COLOR_D_RED | ILI9486_COLOR_D_GREEN | ILI9486_COLOR_D_BLUE },
    { rgb: ILI9486_COLOR_BLACK },
    { rgb: ILI9486_COLOR_L_BLUE },
    { rgb: ILI9486_COLOR_L_GREEN },
    { rgb: ILI9486_COLOR_L_BLUE | ILI9486_COLOR_L_GREEN },
    { rgb: ILI9486_COLOR_L_RED },
    { rgb: ILI9486_COLOR_L_RED | ILI9486_COLOR_L_BLUE },
    { rgb: ILI9486_COLOR_L_RED | ILI9486_COLOR_L_GREEN },
    { rgb: ILI9486_COLOR_L_RED | ILI9486_COLOR_L_GREEN | ILI9486_COLOR_L_BLUE }
};

static ili9486_config_t ili9486_default_config = {
    view: {
        left: 0,
        top: 0,
        width: ILI9486_DEFAULT_WIDTH,
        height: ILI9486_DEFAULT_HEIGHT
    },
    hardware: {
        bitmap_extra_size: 0,
        bpp: 16,
        default_format: DEVICE_COLOR_RGB555,
        palette: ili9486_default_palette,
        palette_count: ILI9486_PALETTE_SIZE,
        type: ILI9486_TYPE_480x320
    },

    width: ILI9486_DEFAULT_WIDTH,
    height: ILI9486_DEFAULT_HEIGHT,
//    bpp: 16,
    bus: 0,
    rst_io_num: GPIO_NUM_NC,
    backlight_io_num: GPIO_NUM_NC,
    column_offset: 0,
    page_offset: 0,

    command_pwctr1: {
        data00: 0x0d,
        data01: 0x0d
    },

    command_pwctr2: {
        data00: 0x43,
        data01: 0x00
    },

    command_pwctr3: {
        data00: 0x00
    },

    command_vmctr1: {
        data00: 0x00,
        data01: 0x48
    },

    command_madctl: {
        dontcare: 0,
        mh: 0,
        bgr: 0,
        ml: 0,
        mv: 1,
        mx: 1,
        my: 0
//        data00: 0x60 // 0x68
    },

    command_pixfmt: {
        data00: 0x55
    },

    command_dfunctr: {
        data00: 0x00,
        data01: 0x22,
        data02: 0x3b
    },

    command_f2: {
        data00: 0x18,
        data01: 0xa3,
        data02: 0x12,
        data03: 0x02,
        data04: 0xb2,
        data05: 0x12,
        data06: 0xff,
        data07: 0x10,
        data08: 0x00
    },

    command_f8: {
        data00: 0x21,
        data01: 0x04,
    },

    command_f9: {
        data00: 0x00,
        data01: 0x08,
    },

    command_gmctrp1: {
        data00: 0x0f,
        data01: 0x24,
        data02: 0x1c,
        data03: 0x0a,
        data04: 0x0f,
        data05: 0x08,
        data06: 0x43,
        data07: 0x88,
        data08: 0x32,
        data09: 0x0f,
        data0a: 0x10,
        data0b: 0x06,
        data0c: 0x0f,
        data0d: 0x07,
        data0e: 0x00
    },

    command_gmctrm1: {
        data00: 0x0f,
        data01: 0x38,
        data02: 0x30,
        data03: 0x09,
        data04: 0x0f,
        data05: 0x0f,
        data06: 0x4e,
        data07: 0x77,
        data08: 0x3c,
        data09: 0x07,
        data0a: 0x10,
        data0b: 0x05,
        data0c: 0x23,
        data0d: 0x1b,
        data0e: 0x00
    },
};


static esp_err_t ili9486_get_default_config(ili9486_config_t *config, ili9486_type_t type)
{
    memcpy(config, &ili9486_default_config, sizeof(ili9486_config_t));
    return ESP_OK;
}

static esp_err_t ili9486_gpio_init(ili9486_t *dev)
{
    gpio_config_t io_conf;
    ili9486_config_t *cfg = &dev->config;
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

static esp_err_t ili9486_reset(const ili9486_t *dev) {
    const ili9486_config_t *config = &dev->config;
    ets_delay_us(10000);
    ESP_ERROR_CHECK(gpio_set_level(config->rst_io_num, 0));
    ets_delay_us(10000);
    ESP_ERROR_CHECK(gpio_set_level(config->rst_io_num, 1));
    ets_delay_us(150000);
    return ESP_OK;
}

static esp_err_t ili9486_bus_out(const ili9486_t *dev, const ili9486_spibus_command_out_t *cmd)
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

    const ili9486_config_t *cfg = &dev->config;
    xSemaphoreTake(dev->bus_mutex, portMAX_DELAY);
    ESP_ERROR_CHECK(cfg->bus->start(cfg->bus, trans, trans_index));
    xSemaphoreGive(dev->bus_mutex);
    return r;
}

static esp_err_t __attribute__((unused)) ili9486_bus_in(const ili9486_t *dev, ili9486_spibus_command_in_t *cmd)
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

    const ili9486_config_t *cfg = &dev->config;
    xSemaphoreTake(dev->bus_mutex, portMAX_DELAY);
    ESP_ERROR_CHECK(cfg->bus->start(cfg->bus, trans, trans_index));
    xSemaphoreGive(dev->bus_mutex);
    return r;
}

static esp_err_t ili9486_bus_out_command(const ili9486_t *dev, uint8_t command)
{
    esp_err_t r = ESP_OK;
    ili9486_spibus_command_out_t cmd = {
        command: command,
        length: 0,
        data: NULL
    };
    ESP_ERROR_CHECK(ili9486_bus_out(dev, &cmd));
    return r;
}

// SLPIN (10h): Sleep in 
static esp_err_t __attribute__((unused)) ili9486_command_sleep_in(const ili9486_t *device)
{
    return ili9486_bus_out_command(device, ILI9486_COMMAND_SLPIN);
}

// SLPOUT (11h): Sleep Out
static esp_err_t ili9486_command_sleep_out(const ili9486_t *device)
{
    return ili9486_bus_out_command(device, ILI9486_COMMAND_SLPOUT);
}


static esp_err_t  __attribute__((unused)) ili9486_command_inversion_on(const ili9486_t *device) {
    return ili9486_bus_out_command(device, ILI9486_COMMAND_INVON);
}

static esp_err_t ili9486_command_inversion_off(const ili9486_t *device) {
    return ili9486_bus_out_command(device, ILI9486_COMMAND_INVOFF);
}

// DISPON (29h): Display On
static esp_err_t ili9486_command_display_on(const ili9486_t *device) {
    return ili9486_bus_out_command(device, ILI9486_COMMAND_DISPON);
}

// DISPOFF (28h): Display Off 
static esp_err_t __attribute__((unused)) ili9486_command_display_off(const ili9486_t *device) {
    return ili9486_bus_out_command(device, ILI9486_COMMAND_DISPOFF);
}

static esp_err_t ili9486_command_pwctr1(const ili9486_t *device, const ili9486_command_pwctr1_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_PWCTR1,
        length: sizeof(ili9486_command_pwctr1_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_pwctr2(const ili9486_t *device, const ili9486_command_pwctr2_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_PWCTR2,
        length: sizeof(ili9486_command_pwctr2_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_pwctr3(const ili9486_t *device, const ili9486_command_pwctr3_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_PWCTR3,
        length: sizeof(ili9486_command_pwctr3_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_vmctr1(const ili9486_t *device, const ili9486_command_vmctr1_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_VMCTR1,
        length: sizeof(ili9486_command_vmctr1_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_madctl(const ili9486_t *device, const ili9486_command_madctl_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_MADCTL,
        length: sizeof(ili9486_command_madctl_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_pixfmt(const ili9486_t *device, const ili9486_command_pixfmt_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_PIXFMT,
        length: sizeof(ili9486_command_pixfmt_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_dfunctr(const ili9486_t *device, const ili9486_command_dfunctr_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_DFUNCTR,
        length: sizeof(ili9486_command_dfunctr_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_f2(const ili9486_t *device, const ili9486_command_f2_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_F2,
        length: sizeof(ili9486_command_f2_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_f8(const ili9486_t *device, const ili9486_command_f8_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_F8,
        length: sizeof(ili9486_command_f8_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_f9(const ili9486_t *device, const ili9486_command_f9_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_F9,
        length: sizeof(ili9486_command_f9_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_gmctrp1(const ili9486_t *device, const ili9486_command_gmctrp1_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_GMCTRP1,
        length: sizeof(ili9486_command_gmctrp1_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_command_gmctrm1(const ili9486_t *device, const ili9486_command_gmctrm1_t *data)
{
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_GMCTRM1,
        length: sizeof(ili9486_command_gmctrm1_t),
        data: data
    };
    return ili9486_bus_out(device, &cmd);
}

// CASET (2Ah): Column Address Set 
static esp_err_t ili9486_column_address_set(const ili9486_t *device, uint16_t beg, uint16_t end) {
    uint8_t buf[4];
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_CASET,
        length: sizeof(buf),
        data: buf
    };
    beg += device->config.column_offset;
    end += device->config.column_offset;
    buf[0] = (beg >> 8) & 0xff;
    buf[1] = beg & 0xff;
    buf[2] = (end >> 8) & 0xff;
    buf[3] = end & 0xff;
    return ili9486_bus_out(device, &cmd);
}

// RASET (2Bh): Row Address Set
static esp_err_t ili9486_page_address_set(const ili9486_t *device, uint16_t beg, uint16_t end) {
    uint8_t buf[4];
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_PASET,
        length: sizeof(buf),
        data: buf
    };
    beg += device->config.page_offset;
    end += device->config.page_offset;
    buf[0] = (beg >> 8) & 0xff;
    buf[1] = beg & 0xff;
    buf[2] = (end >> 8) & 0xff;
    buf[3] = end & 0xff;
    return ili9486_bus_out(device, &cmd);
}

// RAMWR (2Ch): Memory Write 
static esp_err_t ili9486_memory_write(const ili9486_t *device, uint32_t len, const uint8_t *_data) {
    ili9486_spibus_command_out_t cmd = {
        command: ILI9486_COMMAND_RAMWR,
        length: len,
        data: _data
    };
    return ili9486_bus_out(device, &cmd);
}

static esp_err_t ili9486_chip_init(const ili9486_t *device, const ili9486_config_t *config)
{
    esp_err_t r = ESP_OK;

    ESP_ERROR_CHECK(ili9486_reset(device));
    ESP_ERROR_CHECK(ili9486_command_f2(device, &config->command_f2));
    ESP_ERROR_CHECK(ili9486_command_f8(device, &config->command_f8));
    ESP_ERROR_CHECK(ili9486_command_f9(device, &config->command_f9));
    ESP_ERROR_CHECK(ili9486_command_pwctr1(device, &config->command_pwctr1));
    ESP_ERROR_CHECK(ili9486_command_pwctr2(device, &config->command_pwctr2));
    ESP_ERROR_CHECK(ili9486_command_pwctr3(device, &config->command_pwctr3));
    ESP_ERROR_CHECK(ili9486_command_vmctr1(device, &config->command_vmctr1));
    ESP_ERROR_CHECK(ili9486_command_dfunctr(device, &config->command_dfunctr));
    ESP_ERROR_CHECK(ili9486_command_gmctrp1(device, &config->command_gmctrp1));
    ESP_ERROR_CHECK(ili9486_command_gmctrm1(device, &config->command_gmctrm1));
    ESP_ERROR_CHECK(ili9486_command_inversion_off(device));
    ESP_ERROR_CHECK(ili9486_command_madctl(device, &config->command_madctl));
    ESP_ERROR_CHECK(ili9486_command_pixfmt(device, &config->command_pixfmt));
    ESP_ERROR_CHECK(ili9486_command_sleep_out(device));
    ets_delay_us(120000);
    ESP_ERROR_CHECK(ili9486_command_display_on(device));

    return r;
}

static esp_err_t ili9486_bitmap_output(const ili9486_t *device, const display_bitmap_t *bitmap)
{
    esp_err_t err = ESP_OK;
    const ili9486_config_t *config = &device->config;
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
    ESP_ERROR_CHECK(ili9486_column_address_set(device, lt, rt));
    ESP_ERROR_CHECK(ili9486_page_address_set(device, tp, bt));
    ESP_ERROR_CHECK(ili9486_memory_write(device, (b->width * b->height * bitmap->bpp) >> 3, bitmap->data));
    xSemaphoreGive(device->mutex);
    return err;
}

esp_err_t ili9486_init(ili9486_t *device) {
    esp_err_t err = ESP_OK;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_ARG);
    ili9486_config_t *config = &device->config;

    ESP_ERROR_CHECK(ili9486_gpio_init(device));

    device->bus_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(device->bus_mutex == NULL ? ESP_ERR_NO_MEM : ESP_OK);

    device->mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(device->mutex == NULL ? ESP_ERR_NO_MEM : ESP_OK);

    ESP_ERROR_CHECK(ili9486_chip_init(device, config));

    return err;
}

esp_err_t ili9486_done(ili9486_t *device) {
    esp_err_t err = ESP_OK;

    if (device->mutex)
        vSemaphoreDelete(device->mutex);
    if (device->bus_mutex)
        vSemaphoreDelete(device->bus_mutex);

    return err;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t ili9486_display_get_config(const display_t *display, int type)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_ARG);
    ili9486_t *device = (ili9486_t *)display->device;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_STATE);
    ili9486_config_t *config = &device->config;
    ESP_ERROR_CHECK(ili9486_get_default_config(config, type));

    return r;
}

esp_err_t ili9486_display_init(display_t *display, const display_initialization_t *init)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_ARG);
    ili9486_t *device = (ili9486_t *)display->device;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_STATE);

    ili9486_config_t *config = &device->config;

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

    ESP_ERROR_CHECK(ili9486_init(device));

//    ESP_LOGI(TAG, "display w: %d, h: %d", display->bounds.width, display->bounds.height);

    return r;
}


esp_err_t ili9486_display_done(display_t *display)
{
    esp_err_t r = ESP_OK;
    if (display) {
        ili9486_t *device = (ili9486_t *)display->device;
        ESP_ERROR_CHECK(ili9486_done(device));
        if (display->device)
            free(display->device);
        free(display);
    }
    return r;
}

esp_err_t ili9486_display_refresh(const display_bitmap_t *bitmap)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(bitmap ? ESP_OK : ESP_ERR_INVALID_ARG);
    const display_t *display = bitmap->display;
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_STATE);
    ili9486_t *device = (ili9486_t *)display->device;
    ESP_ERROR_CHECK(device ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(ili9486_bitmap_output(device, bitmap));
    return r;
}

esp_err_t ili9486_create(display_t **pdisplay)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(pdisplay ? ESP_OK : ESP_ERR_INVALID_ARG);

    display_t *display = (display_t *)malloc(sizeof(display_t));
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(display, sizeof(display_t));

    display->device = (ili9486_t *)malloc(sizeof(ili9486_t));
    ESP_ERROR_CHECK(display->device ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(display->device, sizeof(ili9486_t));

    display->background.get = NULL;
    display->background.args = NULL;
    display->get_config = ili9486_display_get_config;
    display->init = ili9486_display_init;
    display->done = ili9486_display_done;
    display->refresh = ili9486_display_refresh;
    *pdisplay = display;
    return r;
}


