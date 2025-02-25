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

#ifndef PARTICLE_H
#define PARTICLE_H

#include <bs/player.h>

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float ox, oy, oz;
    unsigned char type;
    float size;
    float fade;
    RGBA4i color;
} Particle;

void particle_init(void);
void particle_update(float dt);
void particle_render(void);
void particle_create_casing(Player *);
void particle_create(RGBA4i color, float x, float y, float z, float velocity, float velocity_y, int amount,
                     float min_size, float max_size);

typedef struct { Vector3f pos; float value; } Vertex;
typedef struct { unsigned int index; size_t begin, end; Vertex data[]; } Trajectory;
typedef struct { size_t size, length; uint8_t * head; } Projectiles;

extern Projectiles projectiles;

extern bool local_hit_effects;

void trajectories_render_all(void);
void trajectories_reset(void);

#define WIDTH(p) (sizeof(Trajectory) + sizeof(Vertex) * (p).length)
#define NEXT(x, p) ((x) = ((x) + 1) % (p).length)

#endif
