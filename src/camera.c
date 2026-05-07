/*
    Copyright © 2016–2021 ByteBit
    Copyright © 2026 DavidCo113
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

#include <math.h>
#include <string.h>
#include <float.h>

#include <bs/common.h>
#include <bs/cameracontroller.h>
#include <bs/player.h>
#include <bs/map.h>
#include <bs/matrix.h>
#include <bs/camera.h>
#include <bs/config.h>

float frustum[6][4];

Camera camera = {
    .mode       = CAMERAMODE_FPS,
    .r          = {256.0F, 60.0F, 256.0F},
    .v          = {0.0F, 0.0F, 0.0F},
    .size       = 0.8F,
    .height     = 0.8F,
    .eye_height = 0.0F,
    .speed      = CAMERA_DEFAULT_SPEED,
    .rot        = {2.04F, 1.79F},
    .crosshair  = {2.04F, 1.79F},
    .muzzle     = {2.04F, 1.79F},
    .noclip     = false
};

float camera_fov_scaled(void) {
    bool render_fpv = (camera.mode == CAMERAMODE_FPS)
        || ((camera.mode == CAMERAMODE_BODYVIEW || camera.mode == CAMERAMODE_SPECTATOR) &&
            cameracontroller_bodyview_mode);
    int local_id = camera.mode == CAMERAMODE_FPS ? local_player.id : cameracontroller_bodyview_player;

    if (render_fpv && players[local_id].alive && ISSCOPING(&players[local_id]) &&
       !HASBIT(players[local_id].input.keys, INPUT_SPRINT))
        return CAMERA_SCOPE_FOV;

    return camera.fov;
}

float camera_clamp_pitch(float y) {
    float y1 = EPSILON, y2 = camera.mode == CAMERAMODE_FPS && settings.render_player
                           ? 15.0F / 180.0F * PI : EPSILON;

    return clamp(y1, PI - y2, y);
}

void camera_overflow_adjust(void) {
    camera.rot.v       = camera_clamp_pitch(camera.rot.v);
    camera.muzzle.v    = camera_clamp_pitch(camera.muzzle.v);
    camera.crosshair.v = camera_clamp_pitch(camera.crosshair.v);

    if (camera.rot.h > TAU) {
        camera.rot.h       -= TAU;
        camera.muzzle.h    -= TAU;
        camera.crosshair.h -= TAU;
    }

    if (camera.rot.h < 0.0F) {
        camera.rot.h       += TAU;
        camera.muzzle.h    += TAU;
        camera.crosshair.h += TAU;
    }
}

void camera_crosshair_move(float dh, float dv) {
    if (camera.mode == CAMERAMODE_FPS) {
        camera.crosshair.h -= dh;
        camera.crosshair.v += dv;

        float h = settings.deadzone_horiz, v = settings.deadzone_vert;

        if (h <= absf(camera.rot.h - camera.crosshair.h))
            camera.rot.h -= dh;

        if (v <= absf(camera.rot.v - camera.crosshair.v))
            camera.rot.v += dv;

        camera.crosshair.h = clamp(camera.rot.h - h, camera.rot.h + h, camera.crosshair.h);
        camera.crosshair.v = clamp(camera.rot.v - v, camera.rot.v + v, camera.crosshair.v);
    } else {
        camera.rot.h -= dh;
        camera.rot.v += dv;

        camera.crosshair = camera.muzzle = camera.rot;
    }

    camera_overflow_adjust();
}

void camera_apply(void) {
    switch (camera.mode) {
        case CAMERAMODE_FPS:       cameracontroller_fps_render();       break;
        case CAMERAMODE_BODYVIEW:  cameracontroller_bodyview_render();  break;
        case CAMERAMODE_SPECTATOR: cameracontroller_spectator_render(); break;
        case CAMERAMODE_SELECTION: cameracontroller_selection_render(); break;
        case CAMERAMODE_DEATH:     cameracontroller_death_render();     break;
    }
}

void camera_update(float dt) {
    switch (camera.mode) {
        case CAMERAMODE_FPS:       cameracontroller_fps(dt);       break;
        case CAMERAMODE_BODYVIEW:  cameracontroller_bodyview(dt);  break;
        case CAMERAMODE_SPECTATOR: cameracontroller_spectator(dt); break;
        case CAMERAMODE_SELECTION: cameracontroller_selection(dt); break;
        case CAMERAMODE_DEATH:     cameracontroller_death(dt);     break;
    }
}

Vector3f camera_orientation(void)  { return Rodrigues3f(camera.rot);       }
Vector3f muzzle_direction(void)    { return Rodrigues3f(camera.muzzle);    }
Vector3f crosshair_direction(void) { return Rodrigues3f(camera.crosshair); }

void camera_hit_fromplayer(CameraHit * hit, int player_id, float range) {
    Vector3f r = players[player_id].physics.eye;
    Vector3f o = player_id != local_player.id ? players[player_id].orientation : muzzle_direction();

    camera_hit(hit, player_id, r.x, r.y + player_height(&players[player_id]), r.z, o.x, o.y, o.z, range);
}

void camera_hit(CameraHit * hit, int exclude_player, float x, float y, float z, float ray_x, float ray_y,
                float ray_z, float range) {
    camera_hit_mask(hit, exclude_player, x, y, z, ray_x, ray_y, ray_z, range);
}

void camera_hit_mask(CameraHit * hit, int exclude_player, float x, float y, float z,
                     float ox, float oy, float oz, float range) {
    Ray dir = {.origin = {x, y, z}, .direction = {ox, oy, oz}};

    hit->type = CAMERA_HITTYPE_NONE;
    hit->distance = FLT_MAX;

#if HACKS_ENABLED && HACK_WALLHACK
    if (players[local_player.id].tool != TOOL_WEAPON) {
#endif
    int solidvox[3], prevox[3]; Vector3f r = {.x = x, .y = y, .z = z}, o = {.x = ox, .y = oy, .z = oz};

    if (camera_terrain_pickEx(r, o, solidvox, prevox) &&
        norm3f(x, y, z, solidvox[X], solidvox[Y], solidvox[Z]) <= sqrf(range)) {
        AABB block = {
            .min = {solidvox[X], solidvox[Y], solidvox[Z]},
            .max = {solidvox[X] + 1, solidvox[Y] + 1, solidvox[Z] + 1},
        };

        float d;
        if (aabb_intersection_ray(&block, &dir, &d)) {
            hit->type     = CAMERA_HITTYPE_BLOCK;
            hit->distance = d;
            hit->x        = solidvox[X];
            hit->y        = solidvox[Y];
            hit->z        = solidvox[Z];
            hit->xb       = prevox[X];
            hit->yb       = prevox[Y];
            hit->zb       = prevox[Z];
        }
    }
#if HACKS_ENABLED && HACK_WALLHACK
    }
#endif

    for (int i = 0; i < PLAYERS_MAX; i++) {
        float l = norm2f(x, z, players[i].pos.x, players[i].pos.z);
        if (players[i].connected && players[i].alive && l < range * range
           && (exclude_player < 0 || (exclude_player >= 0 && exclude_player != i))) {
            Hit intersects = {0};
            player_collision(players + i, &dir, &intersects);

            float d; HitType type = player_intersection_choose(&intersects, &d);
            if (player_intersection_exists(&intersects) && d < hit->distance) {
                hit->type           = CAMERA_HITTYPE_PLAYER;
                hit->distance       = d;
                hit->x              = players[i].pos.x;
                hit->y              = players[i].pos.y;
                hit->z              = players[i].pos.z;
                hit->player_id      = i;
                hit->player_section = type;
            }
        }
    }
}

bool camera_terrain_pick(int solidvox[3], int prevox[3]) {
    Vector3f r = camera.r, o = muzzle_direction();
    return camera_terrain_pickEx(r, o, solidvox, prevox);
}

static inline int taxinorm3i(int x1, int y1, int z1, int x2, int y2, int z2)
{ return abs(x1 - x2) + abs(y1 - y2) + abs(z1 - z2); }

/* https://www.eecs.yorku.ca/~amana/research/grid.pdf
 * https://github.com/fenomas/fast-voxel-raycast
 * https://voxel.wiki/wiki/raytracing/
 * https://voxel.wiki/wiki/raycasting/
 * https://gamedev.stackexchange.com/questions/81267
 */
bool camera_terrain_pickEx(Vector3f r, Vector3f o, int solidvox[3], int prevox[3]) {
    /* Direction in each axis the ray travels. */
    int dx = o.x < 0 ? -1 : 1, dy = o.y < 0 ? -1 : 1, dz = o.z < 0 ? -1 : 1;
    Vector3f tdelta = {.x = fabsf(1 / o.x), .y = fabsf(1 / o.y), .z = fabsf(1 / o.z)};

    /* 1. f(s) = floor(s) + 1 is the smallest integer that is greater than `s`.
       2. g(s) = ceil(s) is the smallest integer that is greater than or equal to `s`.
       In other words, we have that f(s) > s but only g(s) ≥ s. */
    float cx = o.x < 0 ? ceilf(r.x) - 1 : floorf(r.x) + 1;
    float cy = o.y < 0 ? ceilf(r.y) - 1 : floorf(r.y) + 1;
    float cz = o.z < 0 ? ceilf(r.z) - 1 : floorf(r.z) + 1;

    /* Hence, all coordinates of `tmax` are positive. */
    Vector3f tmax = {
        .x = o.x == 0 ? HUGE_VAL : (cx - r.x) / o.x,
        .y = o.y == 0 ? HUGE_VAL : (cy - r.y) / o.y,
        .z = o.z == 0 ? HUGE_VAL : (cz - r.z) / o.z
    };

    Vector3i curr = {.x = floorf(r.x), .y = floorf(r.y), .z = floorf(r.z)}, prev = curr;

    /* No clue why each axis of `o` is (128^-1)x what it should be. */
    int ex = floorf(r.x + o.x * 128);
    int ey = floorf(r.y + o.y * 128);
    int ez = floorf(r.z + o.z * 128);

    for (int d = taxinorm3i(curr.x, curr.y, curr.z, ex, ey, ez); d >= 0; d--) {
        if (curr.x < 0 || curr.x >= map_size_x)
            return false;

        if (curr.y < 0)
            return false;

        if (curr.z < 0 || curr.z >= map_size_z)
            return false;

        if (!map_isair(curr.x, curr.y, curr.z)) {
            if (solidvox != NULL) {
                solidvox[0] = curr.x;
                solidvox[1] = curr.y;
                solidvox[2] = curr.z;
            }

            if (prevox != NULL) {
                prevox[0] = prev.x;
                prevox[1] = prev.y;
                prevox[2] = prev.z;
            }

            return true;
        }

        prev = curr;

        if (tmax.z <= tmax.x && tmax.z <= tmax.y) {
            curr.z += dz;
            tmax.z += tdelta.z;
        } else if (tmax.x < tmax.y) {
            curr.x += dx;
            tmax.x += tdelta.x;
        } else {
            curr.y += dy;
            tmax.y += tdelta.y;
        }
    }

    return false;
}

void camera_ExtractFrustum(void) {
    float clip[16];
    float t;

    mat4 mvp;
    matrix_load(mvp, matrix_model);
    matrix_multiply(mvp, matrix_view);
    matrix_multiply(mvp, matrix_projection);
    memcpy(clip, (float *) mvp, 16 * sizeof(float));

    /* Extract the numbers for the RIGHT plane */
    frustum[0][0] = clip[3]  - clip[0];
    frustum[0][1] = clip[7]  - clip[4];
    frustum[0][2] = clip[11] - clip[8];
    frustum[0][3] = clip[15] - clip[12];

    /* Normalize the result */
    t = hypot3f(frustum[0][0], frustum[0][1], frustum[0][2]);
    frustum[0][0] /= t;
    frustum[0][1] /= t;
    frustum[0][2] /= t;
    frustum[0][3] /= t;

    /* Extract the numbers for the LEFT plane */
    frustum[1][0] = clip[3]  + clip[0];
    frustum[1][1] = clip[7]  + clip[4];
    frustum[1][2] = clip[11] + clip[8];
    frustum[1][3] = clip[15] + clip[12];

    /* Normalize the result */
    t = hypot3f(frustum[1][0], frustum[1][1], frustum[1][2]);
    frustum[1][0] /= t;
    frustum[1][1] /= t;
    frustum[1][2] /= t;
    frustum[1][3] /= t;

    /* Extract the BOTTOM plane */
    frustum[2][0] = clip[3]  + clip[1];
    frustum[2][1] = clip[7]  + clip[5];
    frustum[2][2] = clip[11] + clip[9];
    frustum[2][3] = clip[15] + clip[13];

    /* Normalize the result */
    t = hypot3f(frustum[2][0], frustum[2][1], frustum[2][2]);
    frustum[2][0] /= t;
    frustum[2][1] /= t;
    frustum[2][2] /= t;
    frustum[2][3] /= t;

    /* Extract the TOP plane */
    frustum[3][0] = clip[3]  - clip[1];
    frustum[3][1] = clip[7]  - clip[5];
    frustum[3][2] = clip[11] - clip[9];
    frustum[3][3] = clip[15] - clip[13];

    /* Normalize the result */
    t = hypot3f(frustum[3][0], frustum[3][1], frustum[3][2]);
    frustum[3][0] /= t;
    frustum[3][1] /= t;
    frustum[3][2] /= t;
    frustum[3][3] /= t;

    /* Extract the FAR plane */
    frustum[4][0] = clip[3]  - clip[2];
    frustum[4][1] = clip[7]  - clip[6];
    frustum[4][2] = clip[11] - clip[10];
    frustum[4][3] = clip[15] - clip[14];

    /* Normalize the result */
    t = hypot3f(frustum[4][0], frustum[4][1], frustum[4][2]);
    frustum[4][0] /= t;
    frustum[4][1] /= t;
    frustum[4][2] /= t;
    frustum[4][3] /= t;

    /* Extract the NEAR plane */
    frustum[5][0] = clip[3]  + clip[2];
    frustum[5][1] = clip[7]  + clip[6];
    frustum[5][2] = clip[11] + clip[10];
    frustum[5][3] = clip[15] + clip[14];

    /* Normalize the result */
    t = hypot3f(frustum[5][0], frustum[5][1], frustum[5][2]);
    frustum[5][0] /= t;
    frustum[5][1] /= t;
    frustum[5][2] /= t;
    frustum[5][3] /= t;
}

unsigned char camera_PointInFrustum(float x, float y, float z) {
    int p;

    for (p = 0; p < 6; p++)
        if (frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= 0)
            return 0;

    return 1;
}

int camera_CubeInFrustum(float x, float y, float z, float size, float size_y) {
    int p;
    int c;
    int c2 = 0;

    for (p = 0; p < 6; p++) {
        c = 0;
        if (frustum[p][0] * (x - size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
            c++;
        if (frustum[p][0] * (x + size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
            c++;
        if (frustum[p][0] * (x - size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
            c++;
        if (frustum[p][0] * (x + size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
            c++;
        if (frustum[p][0] * (x - size) + frustum[p][1] * (y) + frustum[p][2] * (z + size) + frustum[p][3] > 0)
            c++;
        if (frustum[p][0] * (x + size) + frustum[p][1] * (y) + frustum[p][2] * (z + size) + frustum[p][3] > 0)
            c++;
        if (frustum[p][0] * (x - size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z + size) + frustum[p][3] > 0)
            c++;
        if (frustum[p][0] * (x + size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z + size) + frustum[p][3] > 0)
            c++;
        if (c == 0)
            return 0;
        if (c == 8)
            c2++;
    }

    return (c2 == 6) ? 2 : 1;
}
