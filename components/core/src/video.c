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
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "cpu.h"
#include "keyboard.h"
#include "computer.h"
#include "video.h"

#define VIDEO_QUEUE_SIZE 0x1000

typedef struct video_queue_data {
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
} video_queue_data_t;

typedef union video_address {
    uint16_t addr;
    struct {
        uint8_t y;
        uint8_t x;
    };
} video_address_t;

static const char *TAG = "video";

static inline uint32_t video_get_colors(uint8_t page0, uint8_t page1, uint8_t port) {
    size_t i;
    uint32_t colors = 0;
    switch (port & 7) {
        case 0:
            for (i = 0; i < 8; ++i) {
                colors <<= 4;
                colors |= (page0 & 1) ? VIDEO_COLOR_GREEN : VIDEO_COLOR_BLACK;
                page0 >>= 1;
            }
            break;
        case 1:
            for (i = 0; i < 8; ++i) {
                colors <<= 4;
                colors |= (page0 & 1) ? VIDEO_COLOR_LCYAN : VIDEO_COLOR_LBLUE;
                page0 >>= 1;
            }
            break;
        case 2:
        case 3:
            for (i = 0; i < 8; ++i) {
                colors <<= 4;
                colors |= VIDEO_COLOR_BLACK;
            }
            break;
        case 4:
            for (i = 0; i < 8; ++i) {
                colors <<= 4;
                if (page1 & 1)
                    colors |= (page0 & 1) ? VIDEO_COLOR_BLUE : VIDEO_COLOR_RED;
                else
                    colors |= (page0 & 1) ? VIDEO_COLOR_GREEN : VIDEO_COLOR_BLACK;
                page0 >>= 1;
                page1 >>= 1;
            }
            break;
        case 5:
            for (i = 0; i < 8; ++i) {
                colors <<= 4;
                if (page1 & 1)
                    colors |= (page0 & 1) ? VIDEO_COLOR_BLUE : VIDEO_COLOR_RED;
                else
                    colors |= (page0 & 1) ? VIDEO_COLOR_GREEN : VIDEO_COLOR_BLACK;
                page0 >>= 1;
                page1 >>= 1;
            }
            break;
        case 6:
        case 7: {
            uint32_t lo = page1 & 0x0f;
            uint32_t hi = (page1>>4) & 0x0f;
            for (i = 0; i < 8; ++i) {
                colors <<= 4;
                colors |= (page0 & 1) ? lo : hi;
                page0 >>= 1;
            }
            break;
        }
    }
    return colors;
}

static int video_mode(const display_point_t *p, const display_refresh_info_t *info, void *color)
{
    ESP_ERROR_CHECK(info ? ESP_OK : ESP_ERR_INVALID_ARG);
    computer_t *cmp = (computer_t *)info->color.args;
    memory_t *mem = cmp->mem;
    const display_bitmap_t *bitmap = info->bitmap;
    const display_t *display = bitmap->display;
//    ESP_ERROR_CHECK(display ? ESP_OK : ESP_ERR_INVALID_ARG);
//    ESP_ERROR_CHECK(bitmap ? ESP_OK : ESP_ERR_INVALID_ARG);

    //uint32_t dx = win_info->width;
    uint32_t x = bitmap->bounds.left + p->x - (display->bounds.width  - VIDEO_DISPLAY_WIDTH) / 2;
    uint32_t y = bitmap->bounds.top  + p->y - (display->bounds.height - VIDEO_DISPLAY_HEIGHT) / 2;
    uint32_t ofs = (((mem->port_fa & 0x03) << 14) | ((x << 5) & 0x3f00) | y) ^ 0xc000;

    //ESP_LOGI(TAG, "offset 0x%04x", ofs);

    *(uint32_t *)color = video_get_colors(mem->ram_page0[ofs], mem->ram_page1[ofs], mem->port_f8);
    return 1;
}

void video_refresh(computer_t *cmp, uint16_t addr) {
    video_address_t data = {
        addr: addr
    };
    xQueueSend(cmp->video_queue, &data, portMAX_DELAY);
}

static inline void video_refresh_window_int(computer_t *cmp, uint32_t left, uint32_t top, uint32_t width, uint32_t height) {
    const display_t *display = cmp->display;
    display_bitmap_t *canvas;
    ESP_ERROR_CHECK(display_bitmap_create(&canvas));
    display_rectangle_t bounds = {
        left: left + (display->bounds.width - VIDEO_DISPLAY_WIDTH) / 2,
        top: top + (display->bounds.height - VIDEO_DISPLAY_HEIGHT) / 2,
        width: width,
        height: height
    };
    canvas->bounds = bounds;
    canvas->format = DEVICE_COLOR_RGB555;
    ESP_ERROR_CHECK(display_bitmap_init(canvas, display));
    display_refresh_info_t info = {
        rectangle: {
            left: 0,
            top: 0,
            width: width,
            height: height,
        },
        bitmap: canvas,
        color: {
            format: DISPLAY_COLOR_8I4,
            args: cmp,
            get: video_mode
        }
    };
//    ESP_LOGI(TAG, "window %d, %d, %d, %d", left, top, width, height);
//    ESP_ERROR_CHECK(display->refresh(display, &info));
    ESP_ERROR_CHECK(display_bitmap_refresh(&info));
    ESP_ERROR_CHECK(display_refresh(canvas));
    ESP_ERROR_CHECK(display_bitmap_done(canvas));
}

static inline void video_refresh_window(computer_t *cmp, uint32_t left, uint32_t top, uint32_t width, uint32_t height) {
    const int max_width = 64;
    const int max_height = 64;
//    ESP_LOGI(TAG, "video refresh: l: %d, t: %d, w: %d, h: %d", left, top, width, height);

    int l = left;
    int w = width;
    int dw = max_width;
    while (w > 0) {
        if (w < dw)
            dw = w;
        int t = top;
        int h = height;
        int dh = max_height;
        while (h > 0) {
            if (h < dh)
                dh = h;
//            ESP_LOGI(TAG, "block refresh: l: %d, t: %d, w: %d, h: %d", l, t, dw, dh);
            video_refresh_window_int(cmp, l, t, dw, dh);
            t += dh;
            h -= dh;
        }
        l += dw;
        w -= dw;
    }
}

static void video_refresh_process(void *arg) {
    static uint8_t state = 1;

    static uint8_t min_x = 0;
    static uint8_t min_y = 0;
    static uint8_t max_x = 0;
    static uint8_t max_y = 0;

    computer_t *cmp = (computer_t *)arg;
    ESP_ERROR_CHECK(cmp ? ESP_OK : ESP_ERR_INVALID_ARG);
    cpu_t *cpu = cmp->cpu;
    ESP_ERROR_CHECK(cpu ? ESP_OK : ESP_ERR_INVALID_STATE);

    uint8_t mix = 0;
    uint8_t miy = 0;
    uint8_t max = 0;
    uint8_t may = 0;

    video_address_t data;
    while (1) {
        if (xQueueReceive(cmp->video_queue, &data, (TickType_t)1) != pdTRUE) {
            vTaskPrioritySet(NULL, tskIDLE_PRIORITY);
            if (state == 1) continue;
            video_refresh_window(cmp, min_x<<3, min_y, (max_x-min_x+1)<<3, max_y-min_y+1);
            state = 1;
            continue;
        }
        else vTaskPrioritySet(NULL, tskIDLE_PRIORITY+2);

        if (data.addr == 0xffff) {
            video_refresh_window(cmp, 0, 0, VIDEO_DISPLAY_WIDTH, VIDEO_DISPLAY_HEIGHT);
            state = 1;
        }
        else {
            uint8_t is_word_op = cpu->is_word;
            while (1) {
                data.addr &= 0x3fff;
                if (state == 1) {
                    min_x = data.x;
                    max_x = data.x;
                    min_y = data.y;
                    max_y = data.y;
                    state = 0;
                }
                else {
                    mix = min_x;
                    miy = min_y;
                    max = max_x;
                    may = max_y;

                    if (mix > data.x) mix = data.x;
                    if (max < data.x) max = data.x;
                    if (miy > data.y) miy = data.y;
                    if (may < data.y) may = data.y;

                    if ((max - mix > 7) || (may - miy > 63)) {
//                        ESP_LOGI(TAG, "begin  (0x%03x, 0x%02x) 0x%03x, 0x%02x, %d", data.x, data.y, (max - mix), (may - miy), cpu_is_word);
                        video_refresh_window(cmp, min_x<<3, min_y, (max_x-min_x+1)<<3, max_y-min_y+1); 
                        min_x = data.x;
                        max_x = data.x;
                        min_y = data.y;
                        max_y = data.y;
                    }
                    else {
                        min_x = mix;
                        min_y = miy;
                        max_x = max;
                        max_y = may;
                    }
                }
                if (is_word_op) {
                    data.addr += 1;
                    is_word_op = 0;
                    continue;
                }
                break;
            }
        }
    }
}


esp_err_t video_init(computer_t *cmp) {

    ESP_ERROR_CHECK(cmp ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(cmp->display ? ESP_OK : ESP_ERR_INVALID_STATE);

    cmp->video_queue = xQueueCreate(VIDEO_QUEUE_SIZE, sizeof(video_address_t));
    ESP_ERROR_CHECK(cmp->video_queue ? ESP_OK : ESP_ERR_NO_MEM);

    BaseType_t result = xTaskCreate(video_refresh_process, TAG, 2048, cmp, tskIDLE_PRIORITY, &cmp->video_task);
    ESP_ERROR_CHECK(result == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);

    return ESP_OK;
}

esp_err_t video_step(computer_t *cmp) {
    memory_t *mem = cmp->mem;
    if (mem->set_video_mode) {
        mem->set_video_mode = false;
        video_refresh(cmp, 0xffff);
//        ESP_LOGI(TAG, "port 0xF8: 0x%02x, pc: 0x%04x", comp_port_f8, cpu_pc);
    }
    if (mem->set_video_buf) {
        mem->set_video_buf = false;
        video_refresh(cmp, 0xffff);
//        ESP_LOGI(TAG, "port 0xFA: 0x%02x, pc: 0x%04x", comp_port_fa, cpu_pc);
    }
    if (mem->video_addr) {
        video_refresh(cmp, mem->video_addr);
        mem->video_addr = 0;
    }

    return ESP_OK;
}


