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

#ifndef UNICODE_H
#define UNICODE_H

#include <ctype.h>

#include <bs/common.h>

#define OCT1(c) ((((uint8_t) c) & 0x80) == 0x00)
#define OCT2(c) ((((uint8_t) c) & 0xE0) == 0xC0)
#define OCT3(c) ((((uint8_t) c) & 0xF0) == 0xE0)
#define OCT4(c) ((((uint8_t) c) & 0xF8) == 0xF0)
#define CONT(c) ((((uint8_t) c) & 0xC0) == 0x80)

uint8_t encodeSize(Codepage, uint32_t);
void encode(Codepage, uint8_t *, uint32_t);

uint8_t decodeSize(Codepage, const uint8_t);
uint32_t decode(Codepage, const uint8_t *);

void convert(char * dest, size_t outsize, Codepage outpage,
             const char * src, size_t insize, Codepage inpage);

size_t encodeMagic(char * dest, size_t outsize, const char * src, size_t insize);
void decodeMagic(char * dest, size_t outsize, const char * src, size_t insize);

static inline bool isprintuni(uint8_t b1)
{ return b1 <= 0x7F ? isprint(b1) : b1 <= 0xF7; }

void strcatprint(char *, size_t, const char *);

#endif
