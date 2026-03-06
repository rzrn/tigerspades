/*
    Copyright © 2017–2021 ByteBit
    Copyright © 2018 vuolen

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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <bs/common.h>
#include <bs/matrix.h>
#include <bs/window.h>
#include <bs/tracer.h>
#include <bs/model.h>
#include <bs/camera.h>
#include <bs/texture.h>
#include <bs/config.h>
#include <bs/sound.h>
#include <bs/entitysystem.h>

EntitySystem tracers;

void tracer_pvelocity(Vector3f * o, Player * p) {
    o->x = o->x * 256.0F / 32.0F + p->physics.velocity.x;
    o->y = o->y * 256.0F / 32.0F + p->physics.velocity.y;
    o->z = o->z * 256.0F / 32.0F + p->physics.velocity.z;
}

typedef struct { float scalef, minimap_x, minimap_y; } TracerMapInfo;

static bool tracer_map_single(void * obj, void * user) {
    Tracer * t = obj;
    TracerMapInfo * info = user;

    float azimuth = -atan2(t->r.direction[Z], t->r.direction[X]) - HALFPI;
    texture_draw_rotated(
        texture(TEXTURE_TRACER),
        info->minimap_x + t->r.origin[X] * info->scalef,
        info->minimap_y - t->r.origin[Z] * info->scalef,
        15 * info->scalef, 15 * info->scalef, azimuth
    );

    return false;
}

void tracer_map(float scalef, float minimap_x, float minimap_y) {
    entitysys_iterate(
        &tracers,
        &(TracerMapInfo) {
            .scalef    = scalef,
            .minimap_x = minimap_x,
            .minimap_y = minimap_y
        },
        tracer_map_single
    );
}

typedef struct {
    float scalef;
    float minimap_x;
    float minimap_y;
    float azimuth;
    float x0, z0;
} TracerMinimapInfo;

static bool tracer_minimap_single(void * obj, void * user) {
    Tracer * t = obj;
    TracerMinimapInfo * info = user;

    float dx = t->r.origin[X] - info->x0;
    float dz = t->r.origin[Z] - info->z0;

    float x = 64.0F + ROTDX(info->azimuth, dx, dz);
    float y = 64.0F + ROTDZ(info->azimuth, dx, dz);

    if (x > 0.0F && x < 128.0F && y > 0.0F && y < 128.0F) {
        float azimuth = -atan2(t->r.direction[Z], t->r.direction[X]) - HALFPI;

        texture_draw_rotated(
            texture(TEXTURE_TRACER),
            info->minimap_x + x * info->scalef,
            info->minimap_y - y * info->scalef,
            15 * info->scalef, 15 * info->scalef,
            azimuth - info->azimuth
        );
    }

    return false;
}

void tracer_minimap(float scalef, float minimap_x, float minimap_y, float azimuth, float x0, float z0) {
    entitysys_iterate(
        &tracers,
        &(TracerMinimapInfo) {
            .scalef    = scalef,
            .minimap_x = minimap_x,
            .minimap_y = minimap_y,
            .azimuth   = azimuth,
            .x0        = x0,
            .z0        = z0
        },
        tracer_minimap_single
    );
}
void tracer_add(int type, float x, float y, float z, float dx, float dy, float dz) {
    float x0 = x + dx / 4.0F, y0 = y + dy / 4.0F, z0 = z + dz / 4.0F;

    Tracer t = {
        .type        = type,
        .x           = x0,
        .y           = y0,
        .z           = z0,
        .r.origin    = {x0, y0, z0},
        .r.direction = {dx, dy, dz},
        .created     = window_time()
    };

    float len = hypot3f(dx, dy, dz);
    camera_hit(&t.hit, -1, t.x, t.y, t.z, dx / len, dy / len, dz / len, 128.0F);

    entitysys_add(&tracers, &t);
}

static bool tracer_render_single(void * obj, void * user) {
    UNUSED(user);

    Tracer * t = (Tracer *) obj;

    static enum kv6 model_tracer[] = {
        [WEAPON_RIFLE]   = MODEL_SEMI_TRACER,
        [WEAPON_SMG]     = MODEL_SMG_TRACER,
        [WEAPON_SHOTGUN] = MODEL_SHOTGUN_TRACER
    };

    matrix_push(matrix_model);
    matrix_translate(matrix_model, t->r.origin[X], t->r.origin[Y], t->r.origin[Z]);
    matrix_pointAt(matrix_model, t->r.direction[X], t->r.direction[Y], t->r.direction[Z]);
    matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
    matrix_upload();
    kv6_render(&model[model_tracer[t->type]], TEAM_SPECTATOR);
    matrix_pop(matrix_model);

    return false;
}

void tracer_render(void) {
    entitysys_iterate(&tracers, NULL, tracer_render_single);
}

static bool tracer_update_single(void * obj, void * user) {
    Tracer * t = (Tracer *) obj;
    float dt = *(float *) user;

    float len = norm3f(t->x, t->y, t->z, t->r.origin[X], t->r.origin[Y], t->r.origin[Z]);

    // 128.0[m] / 256.0[m/s] = 0.5[s]
    if ((t->hit.type != CAMERA_HITTYPE_NONE && len > sqrf(t->hit.distance)) || window_time() - t->created > 0.5F) {
        if (t->hit.type != CAMERA_HITTYPE_NONE)
            sound_create(SOUND_WORLD, sound(SOUND_IMPACT), t->r.origin[X], t->r.origin[Y], t->r.origin[Z]);

        return true;
    } else {
        t->r.origin[X] += t->r.direction[X] * 32.0F * dt;
        t->r.origin[Y] += t->r.direction[Y] * 32.0F * dt;
        t->r.origin[Z] += t->r.direction[Z] * 32.0F * dt;
    }

    return false;
}

void tracer_update(float dt) {
    entitysys_iterate(&tracers, &dt, tracer_update_single);
}

void tracer_init(void) {
    entitysys_create(&tracers, sizeof(Tracer), PLAYERS_MAX);
}
