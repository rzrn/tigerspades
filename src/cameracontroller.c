/*
    Copyright © 2017–2023 ByteBit
    Copyright © 2019 iamgreaser
    Copyright © 2025 Ashy
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

#include <bs/window.h>
#include <bs/map.h>
#include <bs/player.h>
#include <bs/camera.h>
#include <bs/matrix.h>
#include <bs/cameracontroller.h>
#include <bs/config.h>
#include <bs/hud.h>
#include <bs/sound.h>
#include <bs/weapon.h>

bool cameracontroller_bodyview_mode = false;
int cameracontroller_bodyview_player = 0;
float cameracontroller_bodyview_zoom = 0.0F;

Vector3f cameracontroller_death_velocity;

void cameracontroller_death_init(int player, Vector3f r) {
    camera.mode = CAMERAMODE_DEATH;

    float len = hypot3f(camera.r.x - r.x, camera.r.y - r.y, camera.r.z - r.z);
    cameracontroller_death_velocity.x = (camera.r.x - r.x) / len * 3;
    cameracontroller_death_velocity.y = (camera.r.y - r.y) / len * 3;
    cameracontroller_death_velocity.z = (camera.r.z - r.z) / len * 3;

    cameracontroller_bodyview_player = player;
    cameracontroller_bodyview_zoom   = 0.0F;
}

void cameracontroller_death(float dt) {
    AABB box;
    aabb_set_size(&box, camera.size, camera.height, camera.size);
    aabb_set_center(&box,
        camera.r.x + cameracontroller_death_velocity.x * dt,
        camera.r.y + (cameracontroller_death_velocity.y - dt * 32.0F) * dt,
        camera.r.z + cameracontroller_death_velocity.z * dt
    );

    if (!aabb_intersection_terrain(&box)) {
        cameracontroller_death_velocity.y -= dt * 32.0F;
        camera.r.x += cameracontroller_death_velocity.x * dt;
        camera.r.y += cameracontroller_death_velocity.y * dt;
        camera.r.z += cameracontroller_death_velocity.z * dt;
    } else {
        cameracontroller_death_velocity.x *= +0.5F;
        cameracontroller_death_velocity.y *= -0.5F;
        cameracontroller_death_velocity.z *= +0.5F;

        if (hypot3f(cameracontroller_death_velocity.x,
                    cameracontroller_death_velocity.y,
                    cameracontroller_death_velocity.z) < 0.05F)
            camera.mode = CAMERAMODE_BODYVIEW;
    }
}

void cameracontroller_death_render(void) {
    float x = camera.r.x, y = camera.r.y, z = camera.r.z;
    Vector3f o = players[local_player.id].orientation;

    matrix_lookAt(matrix_view, x, y, z, x + o.x, y + o.y, z + o.z, 0.0F, 1.0F, 0.0F);
}

float last_cy;

void cameracontroller_fps(float dt) {
    if (settings.ads_mode == ZOOM_HOLD && players[local_player.id].tool == TOOL_WEAPON) {
        float fov = button_map.rmb && window_time() - players[local_player.id].start.rmb > ZOOM_HOLD_TIME
                  ? CAMERA_SCOPE_FOV : settings.camera_fov;

        camera.fov = (CAMERA_ZOOM_TIME * camera.fov + dt * fov) / (CAMERA_ZOOM_TIME + dt);
    } else {
        camera.fov = settings.camera_fov;
    }

    players[local_player.id].alive = true;

    bool cooldown = false;
    if (players[local_player.id].tool == TOOL_GRENADE && local_player.grenades == 0) {
        local_player.last_tool = players[local_player.id].tool--;
        cooldown = true;
    }

    if (players[local_player.id].tool == TOOL_WEAPON && local_player.ammo + local_player.ammo_reserved == 0) {
        local_player.last_tool = players[local_player.id].tool--;
        cooldown = true;
    }

    if (players[local_player.id].tool == TOOL_BLOCK && local_player.blocks == 0) {
        local_player.last_tool = players[local_player.id].tool--;
        cooldown = true;
    }

    if (cooldown) player_on_tool_change();

#ifdef USE_TOUCH
    if (!local_player.ammo) {
        hud_ingame.input_keyboard(WINDOW_KEY_RELOAD, WINDOW_PRESS,   0, 0);
        hud_ingame.input_keyboard(WINDOW_KEY_RELOAD, WINDOW_RELEASE, 0, 0);
    }
#endif

    last_cy = players[local_player.id].physics.eye.y - players[local_player.id].physics.velocity.y * 0.4F;

    if (hud_active->render_world && chat_input_mode == CHAT_NO_INPUT) {
        SETBIT(players[local_player.id].input.keys, INPUT_UP,    window_key_down(WINDOW_KEY_UP));
        SETBIT(players[local_player.id].input.keys, INPUT_DOWN,  window_key_down(WINDOW_KEY_DOWN));
        SETBIT(players[local_player.id].input.keys, INPUT_LEFT,  window_key_down(WINDOW_KEY_LEFT));
        SETBIT(players[local_player.id].input.keys, INPUT_RIGHT, window_key_down(WINDOW_KEY_RIGHT));

        if (!settings.toggle_crouch) {
            bool crouch = HASBIT(players[local_player.id].input.keys, INPUT_CROUCH);

            if (crouch && !window_key_down(WINDOW_KEY_CROUCH))
                player_try_uncrouch();

            if (!crouch && window_key_down(WINDOW_KEY_CROUCH))
                player_try_crouch();
        }

        SETBIT(players[local_player.id].input.keys, INPUT_SPRINT, window_key_down(WINDOW_KEY_SPRINT));
        SETBIT(players[local_player.id].input.keys, INPUT_JUMP,   window_key_down(WINDOW_KEY_SPACE));
        SETBIT(players[local_player.id].input.keys, INPUT_SNEAK,  window_key_down(WINDOW_KEY_SNEAK));

        if (window_key_down(WINDOW_KEY_SPACE) && !players[local_player.id].physics.airborne) {
            players[local_player.id].physics.jump = true;
            updateInputData();
        }
    }

    camera.r.x = players[local_player.id].physics.eye.x;
    camera.r.y = players[local_player.id].physics.eye.y + player_height(&players[local_player.id]);
    camera.r.z = players[local_player.id].physics.eye.z;

    if (window_key_down(WINDOW_KEY_SPRINT) && chat_input_mode == CHAT_NO_INPUT) {
        players[local_player.id].item_disabled = window_time();
    } else if (window_time() - players[local_player.id].item_disabled < 0.4F && !players[local_player.id].items_show) {
        players[local_player.id].items_show_start = window_time();
        players[local_player.id].items_show       = true;
    }

    if (0.5F <= window_time() - players[local_player.id].item_showup) {
        bool primary = HASBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY);

        if (button_map.lmb && !primary) {
            players[local_player.id].start.lmb = window_time();

            switch (players[local_player.id].tool) {
                case TOOL_WEAPON: {
                    if (weapon_burst < weapon_firemode_burst(weapon_firemode)) {
                        SETBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY, true);
                        weapon_burst = 0;
                    }

                    break;
                }

                case TOOL_GRENADE: {
                    if (local_player.grenades > 0) {
                        sound_create(SOUND_LOCAL, sound(SOUND_GRENADE_PIN), 0.0F, 0.0F, 0.0F);
                        SETBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY, true);
                    }

                    break;
                }

                default: {
                    SETBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY, true);
                    break;
                }
            }
        }

        if (!button_map.lmb && primary) {
            weapon_burst = 0;

            switch (players[local_player.id].tool) {
                case TOOL_GRENADE: {
                    const float dt = window_time() - players[local_player.id].start.lmb;

                    local_player.grenades--;

                    PacketGrenade contained;

                    contained.player_id   = local_player.id;
                    contained.fuse_length = max(3.0F - dt, 0.0F);

                    Vector3f v = {0.0F, 0.0F, 0.0F};
                    if (contained.fuse_length != 0.0F) {
                        Vector3f * o  = &players[local_player.id].orientation;
                        Vector3f * v0 = &players[local_player.id].physics.velocity;

                        v.x = o->x + v0->x;
                        v.y = o->y + v0->y;
                        v.z = o->z + v0->z;
                    }

                    contained.pos = htonv3f(players[local_player.id].pos);
                    contained.vel = htonov3f(v);

                    sendPacketGrenade(&contained, 0);

                    handlePacketGrenade(&contained); // server won’t loop packet back
                    players[local_player.id].item_showup = window_time();

                    SETBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY, false);
                    break;
                }

                default: {
                    SETBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY, false);
                    break;
                }
            }
        }
    }

    if (players[local_player.id].tool != TOOL_WEAPON || settings.ads_mode == ADS_HOLD)
        SETBIT(players[local_player.id].input.buttons, BUTTON_SECONDARY, button_map.rmb);

    if (HASBIT(players[local_player.id].input.keys, INPUT_SPRINT) || players[local_player.id].items_show) {
        players[local_player.id].input.buttons &= MASKOFF(BUTTON_SECONDARY);
        local_player.drag_active = false;
    }

    if (chat_input_mode != CHAT_NO_INPUT) {
        players[local_player.id].input.keys    &= MASKOFF(INPUT_UP);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_DOWN);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_LEFT);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_RIGHT);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_JUMP);
        players[local_player.id].input.buttons &= MASKOFF(BUTTON_PRIMARY);
    }

    camera.v = players[local_player.id].physics.velocity;

    const float tau = 0.039F; // see “src/player.c”

    camera.muzzle.h = (tau * camera.muzzle.h + dt * camera.crosshair.h) / (tau + dt);
    camera.muzzle.v = (tau * camera.muzzle.v + dt * camera.crosshair.v) / (tau + dt);

    players[local_player.id].orientation = players[local_player.id].orientation_smooth = muzzle_direction();
}

void cameracontroller_fps_render(void) {
    float x = camera.r.x, y = camera.r.y, z = camera.r.z;
    Vector3f o = ISSCOPING(&players[local_player.id]) ? crosshair_direction() : camera_orientation();

    matrix_lookAt(matrix_view, x, y, z, x + o.x, y + o.y, z + o.z, 0.0F, 1.0F, 0.0F);
}

void cameracontroller_bodyview_next(void) {
    for (int k = 0; k < PLAYERS_MAX; k++) {
        if (player_can_spectate(&players[cameracontroller_bodyview_player]))
            return;

        cameracontroller_bodyview_inc();
    }

    cameracontroller_bodyview_mode = false;
}

void cameracontroller_bodyview_prev(void) {
    for (int k = 0; k < PLAYERS_MAX; k++) {
        if (player_can_spectate(&players[cameracontroller_bodyview_player]))
            return;

        cameracontroller_bodyview_dec();
    }

    cameracontroller_bodyview_mode = false;
}

void cameracontroller_spectator(float dt) {
    if (cameracontroller_bodyview_mode)
        cameracontroller_bodyview_next();

    if (cameracontroller_bodyview_mode && players[cameracontroller_bodyview_player].alive) {
        Player * p  = &players[cameracontroller_bodyview_player];
        camera.r    = p->physics.eye;
        camera.r.y += player_height(p);
        camera.v    = p->physics.velocity;

        return;
    }

    AABB aabb = {.min = {0, 0, 0}, .max = {0, 0, 0}};
    aabb_set_size(&aabb, camera.size, camera.height, camera.size);
    aabb_set_center(&aabb, camera.r.x, camera.r.y - camera.eye_height, camera.r.z);

    float xd = 0.0F, yd = 0.0F, zd = 0.0F;

    if (chat_input_mode == CHAT_NO_INPUT) {
        float sh = sin(camera.rot.h), ch = cos(camera.rot.h);
        float sv = sin(camera.rot.v), cv = cos(camera.rot.v);

        float nx = sh * sv, ny = cv, nz = ch * sv;
        float rx = -ch,     ry = 0,  rz = sh;

        if (window_key_down(WINDOW_KEY_UP))    { xd += nx; yd += ny; zd += nz; }
        if (window_key_down(WINDOW_KEY_DOWN))  { xd -= nx; yd -= ny; zd -= nz; }
        if (window_key_down(WINDOW_KEY_RIGHT)) { xd += rx; yd += ry; zd += rz; }
        if (window_key_down(WINDOW_KEY_LEFT))  { xd -= rx; yd -= ry; zd -= rz; }

        if (window_key_down(WINDOW_KEY_SPACE))  yd += 1.0F;
        if (window_key_down(WINDOW_KEY_CROUCH)) yd -= 1.0F;
    }

    float absd = hypot3f(xd, yd, zd);

    if (absd > 0.0F) {
        camera.v.x = (xd / absd) * camera.speed;
        camera.v.y = (yd / absd) * camera.speed;
        camera.v.z = (zd / absd) * camera.speed;
    } else {
        float v = hypot3f(camera.v.x, camera.v.y, camera.v.z);

        if (v > 0.0F) {
            const float k = 4.5F, d = 0.8F;

            // dv/dt = −k (v / |v|) |v|^d = −kv / |v|^(1 − d)
            // We’ll get the finite stopping time when d < 1:
            // https://physics.stackexchange.com/questions/801500/would-an-object-stop-if-the-only-force-acting-against-it-is-air-friction
            float dv = -k * dt / pow(v, 1.0F - d);
            camera.v.x += camera.v.x * dv;
            camera.v.y += camera.v.y * dv;
            camera.v.z += camera.v.z * dv;
        }
    }

    float vx = camera.v.x, vy = camera.v.y, vz = camera.v.z;

    if (window_key_down(WINDOW_KEY_SPRINT))
    { vx *= 2.0F; vy *= 2.0F; vz *= 2.0F; }

    float dx = vx * dt, dy = vy * dt, dz = vz * dt;

    float x = modnonnegf(camera.r.x + dx, map_size_x);
    float y = camera.r.y + dy;
    float z = modnonnegf(camera.r.z + dz, map_size_z);

    bool noclip = camera.noclip && camera.mode == CAMERAMODE_SPECTATOR;

    aabb_set_center(&aabb, x, camera.r.y - camera.eye_height, camera.r.z);
    if (!noclip && aabb_intersection_terrain(&aabb))
    { x = camera.r.x; dx = camera.v.x = 0.0F; }

    aabb_set_center(&aabb, x, y - camera.eye_height, camera.r.z);
    if (camera.r.y + dy < 0 || (!noclip && aabb_intersection_terrain(&aabb)))
    { y = camera.r.y; dy = camera.v.y = 0.0F; }

    aabb_set_center(&aabb, x, y - camera.eye_height, z);
    if (!noclip && aabb_intersection_terrain(&aabb))
    { z = camera.r.z; dz = camera.v.z = 0.0F; }

    camera.r.x = x;
    camera.r.y = y;
    camera.r.z = z;
}

void cameracontroller_spectator_render(void) {
    float x = camera.r.x, y = camera.r.y, z = camera.r.z;

    float ox, oy, oz;

    if (cameracontroller_bodyview_mode && players[cameracontroller_bodyview_player].alive) {
        Player * p = &players[cameracontroller_bodyview_player];

        Vector3f * o = settings.smooth_orientation ? &p->orientation_smooth : &p->orientation;

        float n = hypot3f(o->x, o->y, o->z);
        ox = o->x / n; oy = o->y / n; oz = o->z / n;
    } else {
        Vector3f o = camera_orientation();
        ox = o.x; oy = o.y; oz = o.z;
    }

    matrix_lookAt(matrix_view, x, y, z, x + ox, y + oy, z + oz, 0.0F, 1.0F, 0.0F);
}

void cameracontroller_bodyview(float dt) {
    cameracontroller_bodyview_next();

    AABB aabb = {.min = {0, 0, 0}, .max = {0, 0, 0}};
    aabb_set_size(&aabb, 0.4F, 0.4F, 0.4F);

    Player * const p = &players[cameracontroller_bodyview_player];

    Vector3f r = p->pos, o = camera_orientation();
    float h = player_height2(p);

    float k; float traverse_lengths[2] = {-1, -1};
    for (k = 0.0F; k < 5.0F; k += 0.05F) {
        aabb_set_center(&aabb, r.x - o.x * k, r.y - o.y * k + h, r.z - o.z * k);

        if (aabb_intersection_terrain(&aabb) && traverse_lengths[0] < 0)
            traverse_lengths[0] = fmax(k - 0.1F, 0);

        aabb_set_center(&aabb, r.x + o.x * k, r.y + o.y * k + h, r.z + o.z * k);

        if (!aabb_intersection_terrain(&aabb) && traverse_lengths[1] < 0)
            traverse_lengths[1] = fmax(k - 0.1F, 0);
    }

    if (traverse_lengths[0] < 0) traverse_lengths[0] = 5.0F;
    if (traverse_lengths[1] < 0) traverse_lengths[1] = 5.0F;

    float zoom = traverse_lengths[0] <= 0 ? -traverse_lengths[1] : traverse_lengths[0];
    cameracontroller_bodyview_zoom = zoom < cameracontroller_bodyview_zoom
                                   ? zoom : fmin(zoom, cameracontroller_bodyview_zoom + dt * 8.0F);

    // this is needed to determine which chunks need/can be rendered and for sound, minimap etc...
    camera.r.x = r.x - o.x * cameracontroller_bodyview_zoom;
    camera.r.y = r.y - o.y * cameracontroller_bodyview_zoom + h;
    camera.r.z = r.z - o.z * cameracontroller_bodyview_zoom;

    camera.v = p->physics.velocity;

    if (cameracontroller_bodyview_mode && p->alive) {
        camera.r    = p->physics.eye;
        camera.r.y += player_height(p);
        camera.v    = p->physics.velocity;
    }
}

void cameracontroller_bodyview_render(void) {
    Player * const p = &players[cameracontroller_bodyview_player];

    if (cameracontroller_bodyview_mode && players[cameracontroller_bodyview_player].alive) {
        Vector3f r = camera.r, o = p->orientation_smooth;
        float n = hypot3f(o.x, o.y, o.z);

        matrix_lookAt(matrix_view, r.x, r.y, r.z, r.x + o.x / n, r.y + o.y / n, r.z + o.z / n, 0.0F, 1.0F, 0.0F);
    } else {
        Vector3f r = p->pos, o = camera_orientation();
        float h = player_height2(p);

        matrix_lookAt(
            matrix_view,
            r.x - o.x * cameracontroller_bodyview_zoom,
            r.y - o.y * cameracontroller_bodyview_zoom + h,
            r.z - o.z * cameracontroller_bodyview_zoom,
            r.x, r.y + h, r.z, 0.0F, 1.0F, 0.0F
        );
    }
}

void cameracontroller_selection(float dt) {
    UNUSED(dt);

    camera.r = (Vector3f) {256.0F, 79.0F, 256.0F};
    camera.v = (Vector3f) {0.0F, 0.0F, 0.0F};

    matrix_rotate(matrix_view, 90.0F, 1.0F, 0.0F, 0.0F);
    matrix_translate(matrix_view, -camera.r.x, -camera.r.y, -camera.r.z);
}

void cameracontroller_selection_render(void) {
    matrix_rotate(matrix_view, 90.0F, 1.0F, 0.0F, 0.0F);
    matrix_translate(matrix_view, -camera.r.x, -camera.r.y, -camera.r.z);
}
