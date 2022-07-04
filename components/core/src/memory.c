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
 */#include <string.h>
#include "esp_log.h"
#include "memory.h"

static const char __attribute__((unused)) *TAG = "memory";

esp_err_t memory_create(memory_t **pmem)
{
    ESP_ERROR_CHECK(pmem ? ESP_OK : ESP_ERR_INVALID_ARG);
    memory_t *mem = (memory_t *)malloc(sizeof(memory_t));
    ESP_ERROR_CHECK(mem ? ESP_OK : ESP_ERR_NO_MEM);

    mem->ram_page0 = (uint8_t *)malloc(MEMORY_RAM_PAGE0_SIZE);
    ESP_ERROR_CHECK(mem->ram_page0 ? ESP_OK : ESP_ERR_NO_MEM);
    mem->ram_page1 = (uint8_t *)malloc(MEMORY_RAM_PAGE1_SIZE);
    ESP_ERROR_CHECK(mem->ram_page1 ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(mem->ram_page0, MEMORY_RAM_PAGE0_SIZE);
    bzero(mem->ram_page1, MEMORY_RAM_PAGE1_SIZE);

    *pmem = mem;
    return ESP_OK;
}


esp_err_t memory_step(memory_t *mem) {
    if (mem->set_rom_disk) {
        mem->set_rom_disk = false;
        uint16_t addr = *(uint16_t *)&mem->port_f5.b;
        mem->port_f5.a.p = mem->rom_disk[addr];
    }
    return ESP_OK;
}



esp_err_t memory_init(memory_t *mem)
{
    ESP_ERROR_CHECK(mem ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(mem->rom ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(mem->rom_disk ? ESP_OK : ESP_ERR_INVALID_STATE);

    mem->rom_init = false;
    mem->port_f4r.data = 0xffffffff;
    mem->port_f4w.data = 0xffffffff;
    mem->port_f5.data = 0xffffffff;
    mem->port_f6.data = 0xffffffff;
    mem->port_f7.data = 0xffffffff;

    mem->port_f8 = 0;
    mem->port_f9 = 0;
    mem->port_fa = 0;
    mem->port_fb = 0;
    mem->set_keyboard = false;
    mem->set_video_mode = false;
    mem->set_ram_page = false;
    mem->set_video_buf = false;
    mem->set_rom_disk = false;
    mem->video_addr = 0;
    mem->default_read = 0xffffffff;
    mem->default_write = 0xffffffff;

    return ESP_OK;
}

esp_err_t memory_done(memory_t *mem)
{
    ESP_ERROR_CHECK(mem ? ESP_OK : ESP_ERR_INVALID_ARG);
    free(mem->ram_page0);
    free(mem->ram_page1);
    free(mem);
    return ESP_OK;
}



static const uint8_t *memory_get_read_mem_ptr(memory_t *mem, uint16_t addr)
{
    if (mem->rom_init) {
        switch (addr & 0xfc00) {
            case 0xf000:
                return &mem->ram_page0[addr];
            case 0xf400:
                switch (addr & 0x0300) {
                    case 0x0000:
                        return ((uint8_t *)&mem->port_f4r) + (addr & 0x03);
                    case 0x0100:
                        return ((uint8_t *)&mem->port_f5) + (addr & 0x03);
                    case 0x0200:
                        return ((uint8_t *)&mem->port_f6) + (addr & 0x03);
                    case 0x0300:
                        return ((uint8_t *)&mem->port_f7) + (addr & 0x03);
                }
                break;
            case 0xf800:
            case 0xfc00:
                return &mem->rom[addr & 0x7ff];
            default:
                switch(mem->port_f9 & 3) {
                    case 0: return &mem->ram_page0[addr];
                    case 1: return &mem->ram_page1[addr];
                    case 2:
                    case 3: return (uint8_t *)&mem->default_read;
                }
                break;
        }
        return (uint8_t *)&mem->default_read;
    }
    else {
        return &mem->rom[addr & 0x7ff];
    }
}

static uint8_t *memory_get_write_mem_ptr(memory_t *mem, uint16_t addr) {
    switch (addr & 0xfc00) {
        case 0xf000:
            return &mem->ram_page0[addr];
        case 0xf400:
            switch (addr & 0x0300) {
                case 0x0000:
                    mem->set_keyboard = true;
                    return ((uint8_t *)&mem->port_f4w) + (addr & 0x03);
                case 0x0100:
                    mem->set_rom_disk = true;
                    return ((uint8_t *)&mem->port_f5) + (addr & 0x03);
                case 0x0200:
                    return ((uint8_t *)&mem->port_f6) + (addr & 0x03);
                case 0x0300:
                    return ((uint8_t *)&mem->port_f7) + (addr & 0x03);
            }
            break;
        case 0xf800:
            switch (addr & 0x0300) {
                case 0x0000:
                    mem->rom_init = true;
                    mem->set_video_mode = true;
                    return (uint8_t *)&mem->port_f8;
                case 0x0100:
                    mem->set_ram_page = true;
                    return (uint8_t *)&mem->port_f9;
                case 0x0200:
                    mem->set_video_buf = true;
                    return (uint8_t *)&mem->port_fa;
                case 0x0300:
                    return (uint8_t *)&mem->port_fb;
            }
            break;
        case 0xfc00:
            return (uint8_t *)&mem->default_write;
        default:
            if ((addr & 0xc000) == (((mem->port_fa & 3) ^ 3) << 14)) {
                if ((addr & 0x3000) != 0x3000) mem->video_addr = addr;
            }
            switch(mem->port_f9 & 3) {
                case 0: return &mem->ram_page0[addr];
                case 1: return &mem->ram_page1[addr];
                case 2:
                case 3: return (uint8_t *)&mem->default_write;
            }
            break;
    }
    return (uint8_t *)&mem->default_write;
}

const uint8_t *memory_reader_cb(uint16_t addr, void *arg)
{
    memory_t *mem = (memory_t *)arg;
    return memory_get_read_mem_ptr(mem, addr);
}

uint8_t *memory_writer_cb(uint16_t addr, void *arg)
{
    memory_t *mem = (memory_t *)arg;
    return memory_get_write_mem_ptr(mem, addr);
}

