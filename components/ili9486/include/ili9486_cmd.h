/*
 * This file is part of the ili9486 display driver distribution
 * (https://gitlab.romanchenko.su/esp/components/ili9486.git).
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
#ifndef __ILI9486_CMD_H__
#define __ILI9486_CMD_H__


#define ILI9486_COMMAND_SLPIN  0x10
#define ILI9486_COMMAND_SLPOUT 0x11

#define ILI9486_COMMAND_INVOFF   0x20
#define ILI9486_COMMAND_INVON    0x21
#define ILI9486_COMMAND_DISPOFF  0x28
#define ILI9486_COMMAND_DISPON   0x29

#define ILI9486_COMMAND_CASET 0x2A
#define ILI9486_COMMAND_PASET 0x2B
#define ILI9486_COMMAND_RAMWR 0x2C

#define ILI9486_COMMAND_MADCTL   0x36
#define ILI9486_COMMAND_PIXFMT   0x3A

#define ILI9486_COMMAND_DFUNCTR 0xB6

#define ILI9486_COMMAND_PWCTR1 0xC0
#define ILI9486_COMMAND_PWCTR2 0xC1
#define ILI9486_COMMAND_PWCTR3 0xC2
#define ILI9486_COMMAND_VMCTR1 0xC5

#define ILI9486_COMMAND_GMCTRP1 0xE0
#define ILI9486_COMMAND_GMCTRM1 0xE1

#define ILI9486_COMMAND_F2      0xF2
#define ILI9486_COMMAND_F8      0xF8
#define ILI9486_COMMAND_F9      0xF9

#endif // __ILI9486_CMD_H__
