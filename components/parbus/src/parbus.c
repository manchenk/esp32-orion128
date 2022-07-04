/*
 * This file is part of the parallel bus driver distribution
 * (https://gitlab.romanchenko.su/esp/components/parbus.git).
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
#include "sdkconfig.h"

#include "esp32/rom/ets_sys.h"
#include "parbus.h"

inline void par_bus_set_rd(parbus_t *par) {
    GPIO.out_w1ts = par->mask_rd;
}

inline void par_bus_clr_rd(parbus_t *par) {
    GPIO.out_w1tc = par->mask_rd;
}

inline void par_bus_set_wr(parbus_t *par) {
    GPIO.out_w1ts = par->mask_wr;
}

inline void par_bus_clr_wr(parbus_t *par) {
    GPIO.out_w1tc = par->mask_wr;
}

inline void par_bus_set_rs(parbus_t *par) {
    GPIO.out_w1ts = par->mask_rs;
}

inline void par_bus_clr_rs(parbus_t *par) {
    GPIO.out_w1tc = par->mask_rs;
}

inline void par_bus_set_cs(parbus_t *par) {
    GPIO.out_w1ts = par->mask_cs;
}

inline void par_bus_clr_cs(parbus_t *par) {
    GPIO.out_w1tc = par->mask_cs;
}

inline void par_bus_enable(parbus_t *par) {
    GPIO.enable_w1ts = par->mask;
}

inline void par_bus_disable(parbus_t *par) {
    GPIO.enable_w1tc = par->mask;
}

inline void par_bus_out(parbus_t *par, uint8_t data) {
    uint32_t d = data << ((int)par->config.d0);
    GPIO.out_w1ts = d;
    GPIO.out_w1tc = d ^ par->mask;
}

inline uint8_t par_bus_in(parbus_t *par) {
    return GPIO.in >> ((int)par->config.d0);
}

inline void par_bus_delay() {
    for (int i = 0; i < 5; ++i)
        __asm__ __volatile__("   nop");
}

inline void par_bus_out_cycle(parbus_t *par, bool is_cmd, uint8_t byte) {
    if (is_cmd)
        par_bus_clr_rs(par);
    else
        par_bus_set_rs(par);
    par_bus_out(par, byte);
    par_bus_clr_wr(par);
//    par_bus_delay();
    par_bus_set_wr(par);
}

inline void par_bus_out_command(parbus_t *par, uint8_t cmd) {
//    par_bus_clr_rs(par);
    par_bus_out(par, cmd);
    par_bus_clr_wr(par);
//    par_bus_delay();
    par_bus_set_wr(par);
}

inline void par_bus_out_data(parbus_t *par, uint8_t data) {
//    par_bus_set_rs(par);
    par_bus_out(par, data);
    par_bus_clr_wr(par);
//    par_bus_delay();
    par_bus_set_wr(par);
}


inline uint8_t par_bus_in_cycle(parbus_t *par, bool is_cmd) {
    if (is_cmd)
        par_bus_clr_rs(par);
    else
        par_bus_set_rs(par);
    par_bus_clr_rd(par);
//    par_bus_delay();
    uint8_t res = par_bus_in(par);
    par_bus_set_rd(par);
    return res;
}

inline uint8_t par_bus_in_status(parbus_t *par) {
    return par_bus_in_cycle(par, true);
}

inline uint8_t par_bus_in_data(parbus_t *par) {
    return par_bus_in_cycle(par, false);
}


esp_err_t parbus_default_config(parbus_config_t *config)
{
    ESP_ERROR_CHECK(config ? ESP_OK : ESP_ERR_INVALID_ARG);

    config->rd = GPIO_NUM_NC;
    config->wr = GPIO_NUM_NC;
    config->rs = GPIO_NUM_NC;
    config->cs = GPIO_NUM_NC;
    config->d0 = GPIO_NUM_NC;

    return ESP_OK;
}


esp_err_t par_bus_init(parbus_t *par) {
    ESP_ERROR_CHECK(par ? ESP_OK : ESP_ERR_INVALID_ARG);

    ESP_ERROR_CHECK(par->config.rd != GPIO_NUM_NC ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(par->config.wr != GPIO_NUM_NC ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(par->config.rs != GPIO_NUM_NC ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(par->config.cs != GPIO_NUM_NC ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(par->config.d0 != GPIO_NUM_NC ? ESP_OK : ESP_ERR_INVALID_STATE);

    par->mask = 0xffU << ((int)par->config.d0);
    par->mask_rd = 0x1U << ((int)par->config.rd);
    par->mask_wr = 0x1U << ((int)par->config.wr);
    par->mask_rs = 0x1U << ((int)par->config.rs);
    par->mask_cs = 0x1U << ((int)par->config.cs);

    gpio_config_t config_bus = {
        pin_bit_mask: par->mask,
        mode: GPIO_MODE_INPUT_OUTPUT,
        pull_up_en: GPIO_PULLUP_DISABLE,
        pull_down_en: GPIO_PULLDOWN_ENABLE,
        intr_type: GPIO_INTR_DISABLE
    };
    gpio_config(&config_bus);

    gpio_config_t config_ctr = {
        pin_bit_mask: (1U<<(int)par->config.rd) | (1U<<(int)par->config.wr) | (1U<<(int)par->config.rs) | (1U<<(int)par->config.cs),
        mode: GPIO_MODE_OUTPUT,
        pull_up_en: GPIO_PULLUP_DISABLE,
        pull_down_en: GPIO_PULLDOWN_DISABLE,
        intr_type: GPIO_INTR_DISABLE
    };
    gpio_config(&config_ctr);

    par_bus_set_rd(par);
    par_bus_set_wr(par);
    par_bus_set_rs(par);
    par_bus_set_cs(par);
//    par_bus_set_rst();
    return ESP_OK;
}

esp_err_t par_bus_done(parbus_t *par) {
    ESP_ERROR_CHECK(par ? ESP_OK : ESP_ERR_INVALID_ARG);

    return ESP_OK;
}

static esp_err_t par_bus_begin(parbus_t *par)
{
    return ESP_OK;
}

esp_err_t par_bus_io(parbus_t *par, int length, const uint8_t *out_buf, uint8_t *in_buf, bool is_cmd)
{
    int i;
    par_bus_clr_cs(par);
    if (out_buf) {
        const uint8_t *buf = out_buf;
        par_bus_enable(par);
        if (is_cmd) {
            par_bus_clr_rs(par);
            for (i = 0; i < length; ++i) {
                par_bus_out_command(par, *buf++);
            }
        }
        else {
            par_bus_set_rs(par);
            for (i = 0; i < length; ++i) {
                par_bus_out_data(par, *buf++);
            }
        }
    }
    else if (in_buf) {
        par_bus_disable(par);
        for (i = 0; i < length; ++i) {
            in_buf[i] = par_bus_in_cycle(par, is_cmd);
        }
    }
    par_bus_disable(par);
    par_bus_set_cs(par);
    return ESP_OK;
}

static esp_err_t par_bus_end(parbus_t *par)
{
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////
esp_err_t parbus_get_config(bus_t *bus)
{
    esp_err_t ret = ESP_OK;
    ESP_ERROR_CHECK(bus ? ESP_OK : ESP_ERR_INVALID_ARG);
    parbus_config_t *config = (parbus_config_t *)bus->device;
    ESP_ERROR_CHECK(parbus_default_config(config));
    return ret;
}

esp_err_t parbus_init(bus_t *bus)
{
    esp_err_t ret = ESP_OK;
    ESP_ERROR_CHECK(bus ? ESP_OK : ESP_ERR_INVALID_ARG);
    parbus_t *device = (parbus_t *)bus->device;
    ESP_ERROR_CHECK(par_bus_init(device));
    return ret;
}

esp_err_t parbus_done(bus_t *bus)
{
    esp_err_t ret = ESP_OK;
    if (bus) {
        parbus_t *dev = (parbus_t *)bus->device;
        ESP_ERROR_CHECK(par_bus_done(dev));
        free(bus->device);
        free(bus);
    }
    return ret;
}


esp_err_t parbus_start(bus_t *bus, bus_transaction_t *transactions, size_t trans_num)
{
    esp_err_t ret = ESP_OK;
    parbus_t *dev = (parbus_t *)bus->device;
    bus_transaction_t *trans = transactions;
    for (size_t i = 0; i < trans_num; ++i, trans++) {
        switch(trans->state) {
            case BUS_BEGIN:
                ESP_ERROR_CHECK(par_bus_begin(dev));
                break;
            case BUS_COMMAND:
                ESP_ERROR_CHECK(par_bus_io(dev, trans->length, trans->out_data, trans->in_data, true));
                break;
            case BUS_DATA:
                ESP_ERROR_CHECK(par_bus_io(dev, trans->length, trans->out_data, trans->in_data, false));
                break;
            case BUS_END:
                ESP_ERROR_CHECK(par_bus_end(dev));
                break;
        }
    }
    return ret;
}


esp_err_t parbus_create(bus_t **pbus)
{
    esp_err_t ret = ESP_OK;
    ESP_ERROR_CHECK(pbus ? ESP_OK : ESP_ERR_INVALID_ARG);

    bus_t *bus = (bus_t *)malloc(sizeof(bus_t));
    ESP_ERROR_CHECK(bus ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(bus, sizeof(bus_t));

    bus->device = (parbus_t *)malloc(sizeof(parbus_t));
    ESP_ERROR_CHECK(bus->device ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(bus->device, sizeof(parbus_t));

    bus->get_config = parbus_get_config;
    bus->init = parbus_init;
    bus->done = parbus_done;
    bus->start = parbus_start;
    *pbus = bus;
    return ret;
}

