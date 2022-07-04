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
#ifndef __ST7796S_CMD_H__
#define __ST7796S_CMD_H__


#define ST7796S_COMMAND_SLPIN    0x10
#define ST7796S_COMMAND_SLPOUT   0x11
#define ST7796S_COMMAND_INVOFF   0x20
#define ST7796S_COMMAND_INVON    0x21
#define ST7796S_COMMAND_DISPOFF  0x28
#define ST7796S_COMMAND_DISPON   0x29

#define ST7796S_COMMAND_CASET 0x2A
#define ST7796S_COMMAND_PASET 0x2B
#define ST7796S_COMMAND_RAMWR 0x2C

#define ST7796S_COMMAND_13 0x13
#define ST7796S_COMMAND_36 0x36
#define ST7796S_COMMAND_3A 0x3a
#define ST7796S_COMMAND_A7 0xa7
#define ST7796S_COMMAND_B0 0xb0
#define ST7796S_COMMAND_B1 0xb1
#define ST7796S_COMMAND_B4 0xb4
#define ST7796S_COMMAND_B5 0xb5
#define ST7796S_COMMAND_B6 0xb6
#define ST7796S_COMMAND_B7 0xb7
#define ST7796S_COMMAND_C2 0xc2
#define ST7796S_COMMAND_C5 0xc5
#define ST7796S_COMMAND_E0 0xe0
#define ST7796S_COMMAND_E1 0xe1
#define ST7796S_COMMAND_E4 0xe4
#define ST7796S_COMMAND_E8 0xe8
#define ST7796S_COMMAND_F0 0xf0

#endif // __ST7796S_CMD_H__
