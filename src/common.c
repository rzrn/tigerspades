/*
    Copyright © 2023–2025 rzrn

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <string.h>

#include <bs/common.h>

const RGBA4i White   = {0xFF, 0xFF, 0xFF, 0xFF};
const RGBA4i Black   = {0x00, 0x00, 0x00, 0xFF};
const RGBA4i Red     = {0xFF, 0x00, 0x00, 0xFF};
const RGBA4i Green   = {0x00, 0xFF, 0x00, 0xFF};
const RGBA4i Blue    = {0x00, 0x00, 0xFF, 0xFF};
const RGBA4i Yellow  = {0xFF, 0xFF, 0x00, 0xFF};
const RGBA4i Cyan    = {0x00, 0xFF, 0xFF, 0xFF};
const RGBA4i Magenta = {0xFF, 0x00, 0xFF, 0xFF};
const RGBA4i Sky     = {0x80, 0xE8, 0xFF, 0xFF};

const RGB3i Gray = {111, 111, 111};

void writeRGBA(uint32_t * dest, RGBA4i color) {
    *((uint8_t *) dest + 0) = color.r;
    *((uint8_t *) dest + 1) = color.g;
    *((uint8_t *) dest + 2) = color.b;
    *((uint8_t *) dest + 3) = color.a;
}

RGBA4i readRGBA(uint32_t * src) {
    RGBA4i retval;

    retval.r = *((uint8_t *) src + 0);
    retval.g = *((uint8_t *) src + 1);
    retval.b = *((uint8_t *) src + 2);
    retval.a = *((uint8_t *) src + 3);

    return retval;
}

void writeBGR(uint32_t * dest, RGBA4i color) {
    *((uint8_t *) dest + 0) = color.b;
    *((uint8_t *) dest + 1) = color.g;
    *((uint8_t *) dest + 2) = color.r;
    *((uint8_t *) dest + 3) = 255;
}

RGBA4i readBGR(uint32_t * src) {
    RGBA4i retval;

    retval.b = *((uint8_t *) src + 0);
    retval.g = *((uint8_t *) src + 1);
    retval.r = *((uint8_t *) src + 2);
    retval.a = 255;

    return retval;
}

void writeBGRA(uint32_t * dest, RGBA4i color) {
    *((uint8_t *) dest + 0) = color.b;
    *((uint8_t *) dest + 1) = color.g;
    *((uint8_t *) dest + 2) = color.r;
    *((uint8_t *) dest + 3) = color.a;
}

RGBA4i readBGRA(uint32_t * src) {
    RGBA4i retval;

    retval.b = *((uint8_t *) src + 0);
    retval.g = *((uint8_t *) src + 1);
    retval.r = *((uint8_t *) src + 2);
    retval.a = *((uint8_t *) src + 3);

    return retval;
}

void strnzcpy(char * dest, const char * src, size_t size) {
    strncpy(dest, src, size - 1); dest[size - 1] = 0;
}

size_t strsize(const char * buff, size_t maxsize) {
    size_t size = 0;

    while (buff[size] != 0 && size < maxsize)
        size++;

    return size + 1;
}
