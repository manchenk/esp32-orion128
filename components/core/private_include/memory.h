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
 */#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "esp_err.h"

#define MEMORY_RAM_PAGE0_SIZE 0xf400
#define MEMORY_RAM_PAGE1_SIZE 0xf000

typedef union memory_port {
    uint8_t p;
    struct {
        uint8_t p0 : 1;
        uint8_t p1 : 1;
        uint8_t p2 : 1;
        uint8_t p3 : 1;
        uint8_t p4 : 1;
        uint8_t p5 : 1;
        uint8_t p6 : 1;
        uint8_t p7 : 1;
    };
} memory_port_t;

typedef union memory_ports {
    uint32_t data;
    struct {
        memory_port_t a;
        memory_port_t b;
        memory_port_t c;
        memory_port_t ctrl;
    };
} memory_ports_t;


typedef struct memory {
    uint8_t *ram_page0;
    uint8_t *ram_page1;
    const uint8_t *rom;
    const uint8_t *rom_disk;
    bool rom_init;
    memory_ports_t port_f4r;
    memory_ports_t port_f4w;
    memory_ports_t port_f5;
    memory_ports_t port_f6;
    memory_ports_t port_f7;
    uint16_t port_f8;
    uint16_t port_f9;
    uint16_t port_fa;
    uint16_t port_fb;
    bool set_keyboard;
    bool set_video_mode;
    bool set_ram_page;
    bool set_video_buf;
    bool set_rom_disk;
    uint16_t video_addr;
    uint32_t default_read;
    uint32_t default_write;
} memory_t;

const uint8_t *memory_reader_cb(uint16_t addr, void *arg);
uint8_t *memory_writer_cb(uint16_t addr, void *arg);

esp_err_t memory_create(memory_t **pmem);
esp_err_t memory_init(memory_t *mem);
esp_err_t memory_step(memory_t *mem);
esp_err_t memory_done(memory_t *mem);

#endif //__CORE_H__
