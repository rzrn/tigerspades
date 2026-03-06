/*
    Copyright © 2017–2021 ByteBit
    Copyright © 2023–2026 rzrn

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

#ifndef FONT_H
#define FONT_H

#include <stdbool.h>

#include <bs/common.h>

typedef struct _Font Font;

extern Font * font_primary;
extern Font * font_secondary;

void font_init(void);
float font_length(int scale, const char *, int, Codepage);
Vector2f font_render(float x, float y, int scale, const char *, Codepage);
Vector2f font_centered(float x, float y, int scale, const char *, Codepage);

Font * font_select(Font *);

#endif
