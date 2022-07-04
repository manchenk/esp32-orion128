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
#include <string.h>
#include "debug.h"

#include "stdio.h"

void debug_hex_dump(FILE *fout, uint8_t *data, size_t length)
{
    const size_t line_bytes = 16;
    static const char hex[16] = "0123456789abcdef";
    char buf[100] = {0};
    uint8_t *ptr = (uint8_t *)(((uint32_t)data) & ~0x0f);
    fprintf(fout, "%p > %04x\n", data, length);

    while (1) {
        memset(buf, ' ', sizeof(buf));
        sprintf(buf, "%08x", (uint32_t)ptr);
        buf[8] = ':';
        for (size_t i = 0; i < line_bytes; ++i) 
            if ((data <= ptr+i) && (ptr+i < data+length)) {
                buf[10+3*i] = hex[(ptr[i]>>4)&0x0f];
                buf[11+3*i] = hex[ptr[i]&0x0f];
            }
            else {
                buf[10+3*i] = '.';
                buf[11+3*i] = '.';
            }

        for (size_t i = 0; i < line_bytes; ++i) 
            if ((data <= ptr+i) && (ptr+i < data+length))
                buf[10+3*line_bytes+i] = ptr[i] < 0x20 ? '.' : (char)ptr[i];
            else
                buf[10+3*line_bytes+i] = '.';


        buf[10+4*line_bytes] = 0x00;
        fprintf(fout, "%s\n", buf);
        ptr += line_bytes;
        if (ptr > data+length)
            break;
    }
}
