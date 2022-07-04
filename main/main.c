/*
 * This file is part of the esp32-orion128 distribution
 * (https://gitlab.romanchenko.su/esp/esp32/esp32-orion128.git).
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
#include "app.h"

int app_main()
{
    app_t *app;
    ESP_ERROR_CHECK(app_create(&app));
    ESP_ERROR_CHECK(app_init(app));
    ESP_ERROR_CHECK(app_run(app));
    ESP_ERROR_CHECK(app_done(app));
    return 1;
}

