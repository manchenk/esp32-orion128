/*
 * This file is part of the orion128-core distribution
 * (https://gitlab.romanchenko.su/esp/components/orion128-core.git).
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
 */#include <stdio.h>
#include <string.h>
#include "esp_log.h"

#include "computer.h"
#include "keyboard.h"

#define KBD_QUEUE_SIZE 16

#define KBD_KEY_HOME      0x00
#define KBD_KEY_CLEAR     0x01
#define KBD_KEY_ESC       0x02
#define KBD_KEY_F1        0x03
#define KBD_KEY_F2        0x04
#define KBD_KEY_F3        0x05
#define KBD_KEY_F4        0x06
#define KBD_KEY_F5        0x07
#define KBD_KEY_TAB       0x08
#define KBD_KEY_LINEFEED  0x09
#define KBD_KEY_ENTER     0x0a
#define KBD_KEY_BACKSPACE 0x0b
#define KBD_KEY_LEFT      0x0c
#define KBD_KEY_UP        0x0d
#define KBD_KEY_RIGHT     0x0e
#define KBD_KEY_DOWN      0x0f
#define KBD_KEY_0         0x10
#define KBD_KEY_1         0x11
#define KBD_KEY_2         0x12
#define KBD_KEY_3         0x13
#define KBD_KEY_4         0x14
#define KBD_KEY_5         0x15
#define KBD_KEY_6         0x16
#define KBD_KEY_7         0x17
#define KBD_KEY_8         0x18
#define KBD_KEY_9         0x19
#define KBD_KEY_COLON     0x1a
#define KBD_KEY_SEMICOLON 0x1b
#define KBD_KEY_COMMA     0x1c
#define KBD_KEY_MINUS     0x1d
#define KBD_KEY_POINT     0x1e
#define KBD_KEY_SLASH     0x1f
#define KBD_KEY_AT        0x20
#define KBD_KEY_A         0x21
#define KBD_KEY_B         0x22
#define KBD_KEY_C         0x23
#define KBD_KEY_D         0x24
#define KBD_KEY_E         0x25
#define KBD_KEY_F         0x26
#define KBD_KEY_G         0x27
#define KBD_KEY_H         0x28
#define KBD_KEY_I         0x29
#define KBD_KEY_J         0x2a
#define KBD_KEY_K         0x2b
#define KBD_KEY_L         0x2c
#define KBD_KEY_M         0x2d
#define KBD_KEY_N         0x2e
#define KBD_KEY_O         0x2f
#define KBD_KEY_P         0x30
#define KBD_KEY_Q         0x31
#define KBD_KEY_R         0x32
#define KBD_KEY_S         0x33
#define KBD_KEY_T         0x34
#define KBD_KEY_U         0x35
#define KBD_KEY_V         0x36
#define KBD_KEY_W         0x37
#define KBD_KEY_X         0x38
#define KBD_KEY_Y         0x39
#define KBD_KEY_Z         0x3a
#define KBD_KEY_SQ_LEFT   0x3b
#define KBD_KEY_BACKSLASH 0x3c
#define KBD_KEY_SQ_RIGHT  0x3d
#define KBD_KEY_AND       0x3e
#define KBD_KEY_SPACE     0x3f
#define KBD_KEY_US        0x42
#define KBD_KEY_SS        0x44
#define KBD_KEY_RUS       0x48

static const char *TAG = "kbd";

void keyboard_key_press(keyboard_t *kbd, uint8_t key) {
    uint8_t row = (key >> 3) & 0x07;
    uint8_t col = key & 0x07;
    if (key & 0x40) {
        kbd->flags = (~(key << 4)) & 0xf0;
    }
    else {
        kbd->fields[row] = (1 << col);
    }
    kbd->count = 10000;
}

static uint8_t keyboard_translate_key(keyboard_t *kbd, uint32_t key) {
    switch(key) {
        case '[': return KBD_KEY_SQ_LEFT;
        case ']': return KBD_KEY_SQ_RIGHT;
        case '\\': return KBD_KEY_BACKSLASH;
        case '@': return KBD_KEY_AT;
        case ' ': return KBD_KEY_SPACE;
        case ':': return KBD_KEY_COLON;
        case ';': return KBD_KEY_SEMICOLON;
        case 0x1b5b41: return KBD_KEY_UP;
        case 0x1b5b42: return KBD_KEY_DOWN;
        case 0x1b5b44: return KBD_KEY_LEFT;
        case ',': return KBD_KEY_COMMA;
        case '.': return KBD_KEY_POINT;
        case 0x1b5b43: return KBD_KEY_RIGHT; 
        case 0x0008: return KBD_KEY_BACKSPACE;
        case 0x000a: return KBD_KEY_ENTER;
        case 0x0009: return KBD_KEY_TAB;
        case 0x0109: return KBD_KEY_RUS;
        case 0x1b4f50: return KBD_KEY_F1;
        case 0x1b4f51: return KBD_KEY_F2;
        case 0x1b4f52: return KBD_KEY_F3;
        case 0x1b4f53: return KBD_KEY_F4;
        case '`': return KBD_KEY_ESC;
        case 0x1b5b48: {
            kbd->tracing = 1;
            return 0xff;
        }
        case 0x1b5b46: {
            kbd->tracing = 0;
            return 0xff;
        }

        default: {
            if (key >= '0' && key <= '9') return KBD_KEY_0 + key - '0';
            else if (key >= 'a' && key <= 'z') return KBD_KEY_A + key - 'a';
            else {
                ESP_LOGI(TAG, "unknown key: 0x%02x", key);
            }
        }
    }
    return 0xff;
}

static void keyboard_wait_key(void *arg) {
    keyboard_t *kbd = (keyboard_t *)arg;
    ESP_ERROR_CHECK(kbd ? ESP_OK : ESP_ERR_INVALID_ARG);

    uint32_t key = 0;
    int32_t ch;
    while(1) {
        vTaskDelay(5);
        ch = getchar();
        if (ch == -1) continue;
        key |= ch & 0xff;
        if (key == 0x1b || key == 0x1b5b || key == 0x1b4f) {
            key <<= 8;
            continue;
        }
        uint8_t data = keyboard_translate_key(kbd, key);
        if (data != 0xff) {
            xQueueSend(kbd->queue, &data, portMAX_DELAY);
        }
        key = 0;
    }
}


esp_err_t keyboard_create(keyboard_t **pkbd)
{
    ESP_ERROR_CHECK(pkbd ? ESP_OK : ESP_ERR_INVALID_ARG);
    keyboard_t *kbd = (keyboard_t *)malloc(sizeof(keyboard_t));
    ESP_ERROR_CHECK(kbd ? ESP_OK : ESP_ERR_NO_MEM);

    *pkbd = kbd;
    return ESP_OK;
}

esp_err_t keyboard_init(keyboard_t *kbd)
{
    ESP_ERROR_CHECK(kbd ? ESP_OK : ESP_ERR_INVALID_ARG);

    bzero(kbd->fields, sizeof(kbd->fields));
    kbd->flags = 0xff;
    kbd->count = 0;
    kbd->tracing = 0;

    kbd->queue = xQueueCreate(KBD_QUEUE_SIZE, sizeof(uint8_t));
    ESP_ERROR_CHECK(kbd->queue ? ESP_OK : ESP_ERR_NO_MEM);

    BaseType_t result = xTaskCreate(keyboard_wait_key, TAG, 2048, kbd, tskIDLE_PRIORITY, &kbd->task);
    ESP_ERROR_CHECK(result == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    return ESP_OK;
}

esp_err_t keyboard_done(keyboard_t *kbd)
{
    ESP_ERROR_CHECK(kbd ? ESP_OK : ESP_ERR_INVALID_ARG);
    vTaskDelete(kbd->task);
    vQueueDelete(kbd->queue);

    free(kbd);
    return ESP_OK;
}

esp_err_t keyboard_step(keyboard_t *kbd, memory_t *mem)
{
    ESP_ERROR_CHECK(kbd ? ESP_OK : ESP_ERR_INVALID_ARG);
    if (kbd->count) {
        if (--kbd->count == 0) {
            bzero(kbd->fields, sizeof(kbd->fields));
            kbd->flags = mem->port_f4w.c.p | 0xf0;
            mem->port_f4r.b.p = 0xff;
        }
        mem->port_f4r.c.p = (mem->port_f4w.c.p & 0x0f) | kbd->flags;
    }
    else {
        uint8_t data;
        if (xQueueReceive(kbd->queue, &data, (TickType_t)0) == pdTRUE) {
            keyboard_key_press(kbd, data);
        }
    }

    if (mem->set_keyboard) {
        mem->set_keyboard = false;
        uint8_t pa = ~mem->port_f4w.a.p;
        uint8_t pb = 0;
        uint8_t mask = 0x01;
        for (size_t i = 0; i < KEYBOARD_FIELDS_NUM; ++i, mask <<= 1)
            if (pa & mask) pb |= kbd->fields[i];
        mem->port_f4r.b.p = ~pb;

//        ESP_LOGI(kbd_tag, "0: %02x, 1: %02x, 2: %02x, 3: %02x", comp_port_f4[0], comp_port_f4[1], comp_port_f4[2], comp_port_f4[3]);
    }
    return ESP_OK;
}
