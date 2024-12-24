/*
    Copyright (c) 2017-2020 ByteBit

    This file is part of BetterSpades.

    BetterSpades is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BetterSpades is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MINHEAP_H
#define MINHEAP_H

#define pos_key(x, y, z) (((z) << 20) | ((x) << 8) | (y))
#define pos_keyx(key) (((key) >> 8) & 0xFFF)
#define pos_keyy(key) ((key) & 0xFF)
#define pos_keyz(key) (((key) >> 20) & 0xFFF)

typedef struct {
    uint32_t pos;
} MinheapBlock;

typedef struct {
    int index;
    int length;
    MinheapBlock * nodes;
} Minheap;

int int_cmp(void * first_key, void * second_key, size_t key_size);
size_t int_hash(void * raw_key, size_t key_size);

void minheap_create(Minheap * h);
void minheap_clear(Minheap * h);
void minheap_destroy(Minheap * h);
int minheap_isempty(Minheap * h);
MinheapBlock minheap_extract(Minheap * h);
void minheap_set(Minheap * h, MinheapBlock * b, int value);
MinheapBlock * minheap_put(Minheap * h, MinheapBlock * b);

#endif
