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
#ifndef __ILI9486_H__
#define __ILI9486_H__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "bus.h"
#include "display.h"
#include "colors.h"

#define ILI9486_DEFAULT_WIDTH 320
#define ILI9486_DEFAULT_HEIGHT 480

#define ILI9486_COLOR_BLACK 0x0000
#define ILI9486_COLOR_D_RED 0x0f00
#define ILI9486_COLOR_L_RED 0x1f00
#define ILI9486_COLOR_D_BLUE 0x0078
#define ILI9486_COLOR_L_BLUE 0x00f8
#define ILI9486_COLOR_D_GREEN 0xe003
#define ILI9486_COLOR_L_GREEN 0xe007
#define ILI9486_COLOR_BACK 0x0018

#define ILI9486_PALETTE_SIZE 16

typedef union ili9486_color {
    uint16_t rgb;
    struct {
        uint8_t rgb_l;
        uint8_t rgb_h;
    };
} ili9486_color_t;

typedef enum ili9486_type {
    ILI9486_TYPE_480x320 = 0
} ili9486_type_t;

typedef struct ili9486_spibus_command_out {
    uint8_t command;
    size_t length;
    const void *data;
} ili9486_spibus_command_out_t;

typedef struct ili9486_spibus_command_in {
    uint8_t command;
    size_t length;
    void *data;
} ili9486_spibus_command_in_t;

typedef struct ili9486_command_pwctr1 {
    uint8_t data00;
    uint8_t data01;
} ili9486_command_pwctr1_t;

typedef struct ili9486_command_pwctr2 {
    uint8_t data00;
    uint8_t data01;
} ili9486_command_pwctr2_t;

typedef struct ili9486_command_pwctr3 {
    uint8_t data00;
} ili9486_command_pwctr3_t;

typedef struct ili9486_command_vmctr1 {
    uint8_t data00;
    uint8_t data01;
} ili9486_command_vmctr1_t;

typedef struct ili9486_command_madctl {
    uint8_t dontcare: 2;
    uint8_t mh: 1;
    uint8_t bgr: 1;
    uint8_t ml: 1;
    uint8_t mv: 1;
    uint8_t mx: 1;
    uint8_t my: 1;
//    uint8_t data00;
} ili9486_command_madctl_t;

typedef struct ili9486_command_pixfmt {
    uint8_t data00;
} ili9486_command_pixfmt_t;

typedef struct ili9486_command_dfunctr {
    uint8_t data00;
    uint8_t data01;
    uint8_t data02;
} ili9486_command_dfunctr_t;

typedef struct ili9486_command_f2 {
    uint8_t data00;
    uint8_t data01;
    uint8_t data02;
    uint8_t data03;
    uint8_t data04;
    uint8_t data05;
    uint8_t data06;
    uint8_t data07;
    uint8_t data08;
} ili9486_command_f2_t;

typedef struct ili9486_command_f8 {
    uint8_t data00;
    uint8_t data01;
} ili9486_command_f8_t;

typedef struct ili9486_command_f9 {
    uint8_t data00;
    uint8_t data01;
} ili9486_command_f9_t;

typedef struct ili9486_command_gmctrp1 {
    uint8_t data00;
    uint8_t data01;
    uint8_t data02;
    uint8_t data03;
    uint8_t data04;
    uint8_t data05;
    uint8_t data06;
    uint8_t data07;
    uint8_t data08;
    uint8_t data09;
    uint8_t data0a;
    uint8_t data0b;
    uint8_t data0c;
    uint8_t data0d;
    uint8_t data0e;
} ili9486_command_gmctrp1_t;

typedef struct ili9486_command_gmctrm1 {
    uint8_t data00;
    uint8_t data01;
    uint8_t data02;
    uint8_t data03;
    uint8_t data04;
    uint8_t data05;
    uint8_t data06;
    uint8_t data07;
    uint8_t data08;
    uint8_t data09;
    uint8_t data0a;
    uint8_t data0b;
    uint8_t data0c;
    uint8_t data0d;
    uint8_t data0e;
} ili9486_command_gmctrm1_t;

typedef struct ili9486_config {
    display_rectangle_t view;
    display_hardware_config_t hardware;
    int width;
    int height;
    bus_t *bus;
    gpio_num_t rst_io_num;
    gpio_num_t backlight_io_num;
    uint16_t column_offset;
    uint16_t page_offset;
    ili9486_command_pwctr1_t command_pwctr1;
    ili9486_command_pwctr2_t command_pwctr2;
    ili9486_command_pwctr3_t command_pwctr3;
    ili9486_command_vmctr1_t command_vmctr1;
    ili9486_command_madctl_t command_madctl;
    ili9486_command_pixfmt_t command_pixfmt;
    ili9486_command_dfunctr_t command_dfunctr;
    ili9486_command_f2_t command_f2;
    ili9486_command_f8_t command_f8;
    ili9486_command_f9_t command_f9;
    ili9486_command_gmctrp1_t command_gmctrp1;
    ili9486_command_gmctrm1_t command_gmctrm1;
} ili9486_config_t;

typedef struct ili9486 {
    ili9486_config_t config;
    SemaphoreHandle_t bus_mutex;
    SemaphoreHandle_t mutex;
} ili9486_t;

esp_err_t ili9486_create(display_t **pdisp);

#endif // __ILI9486_H__
