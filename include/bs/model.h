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

#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <stdbool.h>

#include <BetterSpades/common.h>
#include <BetterSpades/aabb.h>
#include <BetterSpades/glx.h>
#include <BetterSpades/tesselator.h>

#define KV6_VIS_NEG_X (1 << 0)
#define KV6_VIS_POS_X (1 << 1)
#define KV6_VIS_NEG_Z (1 << 2)
#define KV6_VIS_POS_Z (1 << 3)
#define KV6_VIS_POS_Y (1 << 4)
#define KV6_VIS_NEG_Y (1 << 5)

typedef struct {
    uint16_t x, y, z;
    uint8_t visfaces;
    TrueColor color;
} Voxel;

typedef struct {
    uint16_t xsiz, ysiz, zsiz;
    float xpiv, ypiv, zpiv;
    bool has_display_list, colorize;
    GLXDisplayList display_list[2];
    Voxel * voxels;
    size_t voxel_count;
    float scale;
    float red, green, blue;
} kv6;

enum kv6 {
    MODEL_PLAYERDEAD,
    MODEL_PLAYERHEAD,
    MODEL_PLAYERTORSO,
    MODEL_PLAYERTORSOC,
    MODEL_PLAYERARMS,
    MODEL_PLAYERLEG,
    MODEL_PLAYERLEGC,

    MODEL_INTEL,
    MODEL_TENT,

    MODEL_SEMI,
    MODEL_SMG,
    MODEL_SHOTGUN,
    MODEL_SPADE,
    MODEL_BLOCK,
    MODEL_GRENADE,

    MODEL_SEMI_TRACER,
    MODEL_SMG_TRACER,
    MODEL_SHOTGUN_TRACER,

    MODEL_SEMI_CASING,
    MODEL_SMG_CASING,
    MODEL_SHOTGUN_CASING,

    MODEL_FIRST = MODEL_PLAYERDEAD,
    MODEL_LAST  = MODEL_SHOTGUN_CASING
};

#define MODEL_TOTAL (MODEL_LAST + 1)

extern kv6 model[MODEL_TOTAL];

void kv6_calclight(int x, int y, int z);
void kv6_rebuild_complete(void);
void kv6_rebuild(kv6 *);
void kv6_render(kv6 *, unsigned char team);
void kv6_load(kv6 *, const char *, uint8_t * buff, float scale);
void kv6_init(void);

extern float kv6_normals[256][3];

#endif
