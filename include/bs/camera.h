/*
    Copyright © 2017–2020 ByteBit
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

#ifndef CAMERA_H
#define CAMERA_H

#include <ace/protocol.h>

typedef enum {
    CAMERAMODE_SELECTION,
    CAMERAMODE_FPS,
    CAMERAMODE_SPECTATOR,
    CAMERAMODE_BODYVIEW,
    CAMERAMODE_DEATH,
} CameraMode;

typedef struct {
    CameraMode mode; Vector3f r, v;
    float size, height, eye_height, speed, fov;
    Euler2f rot, crosshair, muzzle;
    bool noclip;
} Camera;

#define CAMERA_ZOOM_TIME 0.20F

#define CAMERA_MINIMUM_SPEED 0.0F
#define CAMERA_DEFAULT_SPEED 32.0F
#define CAMERA_MAXIMUM_SPEED 512.0F

#define CAMERA_SCOPE_FOV   45.0F
#define CAMERA_DEFAULT_FOV 70.0F
#define CAMERA_MAX_FOV     100.0F

extern float frustum[6][4];
extern Camera camera;

typedef struct {
    char type;
    float x, y, z, distance;
    int xb, yb, zb;
    unsigned char player_id;
    HitType player_section;
} CameraHit;

enum {
    CAMERA_HITTYPE_NONE   = 0,
    CAMERA_HITTYPE_BLOCK  = 1,
    CAMERA_HITTYPE_PLAYER = 2
};

Vector3f camera_orientation(void);
Vector3f muzzle_direction(void);
Vector3f crosshair_direction(void);

void camera_hit_fromplayer(CameraHit *, int player_id, float range);
void camera_hit(CameraHit *, int exclude_player, float x, float y, float z, float ray_x, float ray_y,
                float ray_z, float range);
void camera_hit_mask(CameraHit *, int exclude_player, float x, float y, float z, float ray_x,
                     float ray_y, float ray_z, float range);

float camera_fov_scaled(void);

void camera_ExtractFrustum(void);
unsigned char camera_PointInFrustum(float x, float y, float z);
int camera_CubeInFrustum(float x, float y, float z, float size, float size_y);
int * camera_terrain_pick(unsigned char mode);
int * camera_terrain_pickEx(unsigned char mode, float x, float y, float z, float ray_x, float ray_y, float ray_z);
void camera_overflow_adjust(void);
void camera_crosshair_move(float, float);
void camera_apply(void);
void camera_update(float dt);

float camera_clamp_pitch(float);

#endif
