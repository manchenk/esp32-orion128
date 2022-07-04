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
#ifndef __ST7796S_H__
#define __ST7796S_H__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "bus.h"
#include "display.h"
#include "colors.h"

#define ST7796S_DEFAULT_WIDTH 320
#define ST7796S_DEFAULT_HEIGHT 480

#define ST7796S_COLOR_BLACK 0x0000
#define ST7796S_COLOR_D_RED 0x0f00
#define ST7796S_COLOR_L_RED 0x1f00
#define ST7796S_COLOR_D_BLUE 0x0078
#define ST7796S_COLOR_L_BLUE 0x00f8
#define ST7796S_COLOR_D_GREEN 0xe003
#define ST7796S_COLOR_L_GREEN 0xe007
#define ST7796S_COLOR_BACK 0x0018

#define ST7796S_PALETTE_SIZE 16

typedef union st7796s_color {
    uint16_t rgb;
    struct {
        uint8_t rgb_l;
        uint8_t rgb_h;
    };
} st7796s_color_t;

typedef enum st7796s_type {
    ST7796S_TYPE_480x320 = 0
} st7796s_type_t;

typedef struct st7796s_spibus_command_out {
    uint8_t command;
    size_t length;
    const void *data;
} st7796s_spibus_command_out_t;

typedef struct st7796s_spibus_command_in {
    uint8_t command;
    size_t length;
    void *data;
} st7796s_spibus_command_in_t;

typedef struct st7796s_command_f0 {
    uint8_t data00;
} st7796s_command_f0_t;
typedef struct st7796s_command_madctl {
    uint8_t dontcare: 2;
    uint8_t mh: 1;
    uint8_t bgr: 1;
    uint8_t ml: 1;
    uint8_t mv: 1;
    uint8_t mx: 1;
    uint8_t my: 1;
//    uint8_t data00;
} st7796s_command_madctl_t;
typedef struct st7796s_command_3a {
    uint8_t data00;
} st7796s_command_3a_t;
typedef struct st7796s_command_b0 {
    uint8_t data00;
} st7796s_command_b0_t;
typedef struct st7796s_command_b6 {
    uint8_t data00;
    uint8_t data01;
} st7796s_command_b6_t;
typedef struct st7796s_command_b5 {
    uint8_t data00;
    uint8_t data01;
    uint8_t data02;
    uint8_t data03;
} st7796s_command_b5_t;
typedef struct st7796s_command_b1 {
    uint8_t data00;
    uint8_t data01;
} st7796s_command_b1_t;
typedef struct st7796s_command_b4 {
    uint8_t data00;
} st7796s_command_b4_t;
typedef struct st7796s_command_b7 {
    uint8_t data00;
} st7796s_command_b7_t;
typedef struct st7796s_command_c5 {
    uint8_t data00;
} st7796s_command_c5_t;
typedef struct st7796s_command_e4 {
    uint8_t data00;
} st7796s_command_e4_t;
typedef struct st7796s_command_e8 {
    uint8_t data00;
    uint8_t data01;
    uint8_t data02;
    uint8_t data03;
    uint8_t data04;
    uint8_t data05;
    uint8_t data06;
    uint8_t data07;
} st7796s_command_e8_t;
typedef struct st7796s_command_e0 {
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
} st7796s_command_e0_t;
typedef struct st7796s_command_e1 {
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
} st7796s_command_e1_t;


typedef struct st7796s_config {
    display_rectangle_t view;
    display_hardware_config_t hardware;
    int width;
    int height;
//    int bpp;
    bus_t *bus;
    gpio_num_t rst_io_num;
    gpio_num_t backlight_io_num;
    int column_offset;
    int page_offset;

    st7796s_command_f0_t command_f0_0;
    st7796s_command_f0_t command_f0_1;
    st7796s_command_f0_t command_f0_2;
    st7796s_command_f0_t command_f0_3;
    st7796s_command_madctl_t command_madctl;
//    st7796s_command_madctl_t command_madctl_1;
    st7796s_command_3a_t command_3a;
    st7796s_command_b0_t command_b0;
    st7796s_command_b6_t command_b6;
    st7796s_command_b5_t command_b5;
    st7796s_command_b1_t command_b1;
    st7796s_command_b4_t command_b4;
    st7796s_command_b7_t command_b7;
    st7796s_command_c5_t command_c5;
    st7796s_command_e4_t command_e4;
    st7796s_command_e8_t command_e8;
    st7796s_command_e0_t command_e0;
    st7796s_command_e1_t command_e1;

} st7796s_config_t;

typedef struct st7796s {
    st7796s_config_t config;
    SemaphoreHandle_t bus_mutex;
    SemaphoreHandle_t mutex;
} st7796s_t;

esp_err_t st7796s_create(display_t **pdisp);

#endif // __ST7796S_H__
