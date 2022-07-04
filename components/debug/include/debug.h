/*
 * This file is part of the debug component distribution
 * (https://gitlab.romanchenko.su/esp/components/debug.git).
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
#ifndef __DEBUG_H___
#define __DEBUG_H__

#include "esp_log.h"
#include "stdio.h"

#define MEM_ALLOC_LOGI(ptr, size)                                                                          \
    do                                                                                                     \
    {                                                                                                      \
        ESP_LOGI(TAG, "Memory allocation at %s:%d, ptr: %p, size: %d", __FUNCTION__, __LINE__, ptr, size); \
    } while (0)

#define TRACEI(format, ...) ESP_LOGI(TAG, "Trace at %s:%d. " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#endif // __DEBUG_H__
