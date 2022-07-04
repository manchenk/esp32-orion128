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
#include "computer.h"
#include "video.h"


esp_err_t computer_create(computer_t **pcmp)
{
    esp_err_t r = ESP_OK;
    ESP_ERROR_CHECK(pcmp ? ESP_OK : ESP_ERR_INVALID_ARG);
    computer_t *cmp = (computer_t *)malloc(sizeof(computer_t));
    ESP_ERROR_CHECK(cmp ? ESP_OK : ESP_ERR_NO_MEM);
    bzero(cmp, sizeof(computer_t));

    ESP_ERROR_CHECK(cpu_create(&cmp->cpu));
    ESP_ERROR_CHECK(memory_create(&cmp->mem));
    ESP_ERROR_CHECK(keyboard_create(&cmp->kbd));

    *pcmp = cmp;
    return r;
}

esp_err_t computer_init(computer_t *cmp)
{
    ESP_ERROR_CHECK(keyboard_init(cmp->kbd));
    ESP_ERROR_CHECK(video_init(cmp));

    ESP_ERROR_CHECK(memory_init(cmp->mem));

    cpu_t *cpu = cmp->cpu;
    cpu->reader = memory_reader_cb;
    cpu->writer = memory_writer_cb;
    cpu->memory = cmp->mem;
    ESP_ERROR_CHECK(cpu_init(cmp->cpu));

    //comp_init();
    return ESP_OK;
}

esp_err_t computer_step(computer_t *cmp)
{
    ESP_ERROR_CHECK(cpu_step(cmp->cpu));
    ESP_ERROR_CHECK(video_step(cmp));
    ESP_ERROR_CHECK(keyboard_step(cmp->kbd, cmp->mem));
    ESP_ERROR_CHECK(memory_step(cmp->mem));
    return ESP_OK;
}

esp_err_t computer_done(computer_t *cmp)
{
    ESP_ERROR_CHECK(cmp ? ESP_OK : ESP_ERR_INVALID_ARG);
    ESP_ERROR_CHECK(keyboard_done(cmp->kbd));
    ESP_ERROR_CHECK(memory_done(cmp->mem));
    ESP_ERROR_CHECK(cpu_done(cmp->cpu));

    free(cmp);
    return ESP_OK;
}

