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

#include <math.h>

#include <bs/window.h>
#include <bs/map.h>
#include <bs/player.h>
#include <bs/camera.h>
#include <bs/matrix.h>
#include <bs/cameracontroller.h>
#include <bs/config.h>
#include <bs/hud.h>

int cameracontroller_bodyview_mode = 0;
int cameracontroller_bodyview_player = 0;
float cameracontroller_bodyview_zoom = 0.0F;

Vector3f cameracontroller_death_velocity;

void cameracontroller_death_init(int player, Vector3f r) {
    camera.mode = CAMERAMODE_DEATH;

    float len = hypot3f(camera.pos.x - r.x, camera.pos.y - r.y, camera.pos.z - r.z);
    cameracontroller_death_velocity.x = (camera.pos.x - r.x) / len * 3;
    cameracontroller_death_velocity.y = (camera.pos.y - r.y) / len * 3;
    cameracontroller_death_velocity.z = (camera.pos.z - r.z) / len * 3;

    cameracontroller_bodyview_player = player;
    cameracontroller_bodyview_zoom   = 0.0F;
}

void cameracontroller_death(float dt) {
    AABB box;
    aabb_set_size(&box, camera.size, camera.height, camera.size);
    aabb_set_center(&box,
        camera.pos.x + cameracontroller_death_velocity.x * dt,
        camera.pos.y + (cameracontroller_death_velocity.y - dt * 32.0F) * dt,
        camera.pos.z + cameracontroller_death_velocity.z * dt
    );

    if (!aabb_intersection_terrain(&box, 0)) {
        cameracontroller_death_velocity.y -= dt * 32.0F;
        camera.pos.x += cameracontroller_death_velocity.x * dt;
        camera.pos.y += cameracontroller_death_velocity.y * dt;
        camera.pos.z += cameracontroller_death_velocity.z * dt;
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

void cameracontroller_death_render() {
    matrix_lookAt(matrix_view, camera.pos.x, camera.pos.y, camera.pos.z, camera.pos.x + players[local_player.id].orientation.x,
                  camera.pos.y + players[local_player.id].orientation.y, camera.pos.z + players[local_player.id].orientation.z,
                  0.0F, 1.0F, 0.0F);
}

float last_cy;
void cameracontroller_fps(float dt) {
    players[local_player.id].alive = 1;

    int cooldown = 0;
    if (players[local_player.id].held_item == TOOL_GRENADE && local_player.grenades == 0) {
        local_player.last_tool = players[local_player.id].held_item--;
        cooldown = 1;
    }

    if (players[local_player.id].held_item == TOOL_GUN && local_player.ammo + local_player.ammo_reserved == 0) {
        local_player.last_tool = players[local_player.id].held_item--;
        cooldown = 1;
    }

    if (players[local_player.id].held_item == TOOL_BLOCK && local_player.blocks == 0) {
        local_player.last_tool = players[local_player.id].held_item--;
        cooldown = 1;
    }

    if (cooldown) player_on_held_item_change(players + local_player.id);

#ifdef USE_TOUCH
    if (!local_player.ammo) {
        hud_ingame.input_keyboard(WINDOW_KEY_RELOAD, WINDOW_PRESS, 0, 0);
        hud_ingame.input_keyboard(WINDOW_KEY_RELOAD, WINDOW_RELEASE, 0, 0);
    }
#endif

    last_cy = players[local_player.id].physics.eye.y - players[local_player.id].physics.velocity.y * 0.4F;

    if (hud_active->render_world && chat_input_mode == CHAT_NO_INPUT) {
        SETBIT(players[local_player.id].input.keys, INPUT_UP,    window_key_down(WINDOW_KEY_UP));
        SETBIT(players[local_player.id].input.keys, INPUT_DOWN,  window_key_down(WINDOW_KEY_DOWN));
        SETBIT(players[local_player.id].input.keys, INPUT_LEFT,  window_key_down(WINDOW_KEY_LEFT));
        SETBIT(players[local_player.id].input.keys, INPUT_RIGHT, window_key_down(WINDOW_KEY_RIGHT));

        if (HASBIT(players[local_player.id].input.keys, INPUT_CROUCH) &&
            !window_key_down(WINDOW_KEY_CROUCH) && player_uncrouch(&players[local_player.id]))
            players[local_player.id].input.keys &= MASKOFF(INPUT_CROUCH);

        if (window_key_down(WINDOW_KEY_CROUCH)) {
            // following if-statement disables smooth crouching on local player
            if (!HASBIT(players[local_player.id].input.keys, INPUT_CROUCH) &&
                !players[local_player.id].physics.airborne) {
                players[local_player.id].pos.y         -= 0.9F;
                players[local_player.id].physics.eye.y -= 0.9F;
                last_cy                                -= 0.9F;
            }

            players[local_player.id].input.keys |= MASKON(INPUT_CROUCH);
        }

        SETBIT(players[local_player.id].input.keys, INPUT_SPRINT, window_key_down(WINDOW_KEY_SPRINT));
        SETBIT(players[local_player.id].input.keys, INPUT_JUMP,   window_key_down(WINDOW_KEY_SPACE));
        SETBIT(players[local_player.id].input.keys, INPUT_SNEAK,  window_key_down(WINDOW_KEY_SNEAK));

        if (window_key_down(WINDOW_KEY_SPACE) && !players[local_player.id].physics.airborne)
            players[local_player.id].physics.jump = 1;
    }

    camera.pos.x = players[local_player.id].physics.eye.x;
    camera.pos.y = players[local_player.id].physics.eye.y + player_height(&players[local_player.id]);
    camera.pos.z = players[local_player.id].physics.eye.z;

    if (window_key_down(WINDOW_KEY_SPRINT) && chat_input_mode == CHAT_NO_INPUT) {
        players[local_player.id].item_disabled = window_time();
    } else if (window_time() - players[local_player.id].item_disabled < 0.4F && !players[local_player.id].items_show) {
        players[local_player.id].items_show_start = window_time();
        players[local_player.id].items_show       = 1;
    }

    SETBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY, button_map.lmb);

    if (players[local_player.id].held_item != TOOL_GUN || settings.hold_down_sights)
        SETBIT(players[local_player.id].input.buttons, BUTTON_SECONDARY, button_map.rmb);

    if (HASBIT(players[local_player.id].input.keys, INPUT_SPRINT) || players[local_player.id].items_show)
        players[local_player.id].input.buttons &= MASKOFF(BUTTON_SECONDARY);

    if (chat_input_mode != CHAT_NO_INPUT) {
        players[local_player.id].input.keys    &= MASKOFF(INPUT_UP);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_DOWN);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_LEFT);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_RIGHT);
        players[local_player.id].input.keys    &= MASKOFF(INPUT_JUMP);
        players[local_player.id].input.buttons &= MASKOFF(BUTTON_PRIMARY);
    }

    float k = pow(0.7F, dt * 60.0F);

    float lx = k * players[local_player.id].orientation_smooth.x + (1.0F - k) * sin(camera.rot.x) * sin(camera.rot.y);
    float ly = k * players[local_player.id].orientation_smooth.y + (1.0F - k) * cos(camera.rot.y);
    float lz = k * players[local_player.id].orientation_smooth.z + (1.0F - k) * cos(camera.rot.x) * sin(camera.rot.y);

    players[local_player.id].orientation_smooth.x = lx;
    players[local_player.id].orientation_smooth.y = ly;
    players[local_player.id].orientation_smooth.z = lz;

    float len = hypot3f(lx, ly, lz);
    players[local_player.id].orientation.x = lx / len;
    players[local_player.id].orientation.y = ly / len;
    players[local_player.id].orientation.z = lz / len;

    camera.v = players[local_player.id].physics.velocity;
}

void cameracontroller_fps_render() {
    matrix_lookAt(matrix_view, camera.pos.x, camera.pos.y, camera.pos.z, camera.pos.x + sin(camera.rot.x) * sin(camera.rot.y),
                  camera.pos.y + cos(camera.rot.y), camera.pos.z + cos(camera.rot.x) * sin(camera.rot.y), 0.0F, 1.0F, 0.0F);
}

void cameracontroller_spectator(float dt) {
    AABB aabb = {.min = {0, 0, 0}, .max = {0, 0, 0}};
    aabb_set_size(&aabb, camera.size, camera.height, camera.size);
    aabb_set_center(&aabb, camera.pos.x, camera.pos.y - camera.eye_height, camera.pos.z);

    float x = 0.0F, y = 0.0F, z = 0.0F;

    if (chat_input_mode == CHAT_NO_INPUT) {
        if (window_key_down(WINDOW_KEY_UP)) {
            x += sin(camera.rot.x) * sin(camera.rot.y);
            y += cos(camera.rot.y);
            z += cos(camera.rot.x) * sin(camera.rot.y);
        } else if (window_key_down(WINDOW_KEY_DOWN)) {
            x -= sin(camera.rot.x) * sin(camera.rot.y);
            y -= cos(camera.rot.y);
            z -= cos(camera.rot.x) * sin(camera.rot.y);
        }

        if (window_key_down(WINDOW_KEY_LEFT)) {
            x += sin(camera.rot.x + 1.57F);
            z += cos(camera.rot.x + 1.57F);
        } else if (window_key_down(WINDOW_KEY_RIGHT)) {
            x += sin(camera.rot.x - 1.57F);
            z += cos(camera.rot.x - 1.57F);
        }

        if (window_key_down(WINDOW_KEY_SPACE)) y++;
        else if (window_key_down(WINDOW_KEY_CROUCH)) y--;
    }

    float len = hypot3f(x, y, z);
    if (len > 0.0F) {
        camera.movement.x = (x / len) * camera.speed * dt;
        camera.movement.y = (y / len) * camera.speed * dt;
        camera.movement.z = (z / len) * camera.speed * dt;
    }

    if (fabsf(camera.movement.x) < 1.0F)
        camera.movement.x *= pow(0.0025F, dt);

    if (fabsf(camera.movement.y) < 1.0F)
        camera.movement.y *= pow(0.0025F, dt);

    if (fabsf(camera.movement.z) < 1.0F)
        camera.movement.z *= pow(0.0025F, dt);

    aabb_set_center(&aabb, camera.pos.x + camera.movement.x, camera.pos.y - camera.eye_height, camera.pos.z);

    if (camera.pos.x + camera.movement.x < 0 || camera.pos.x + camera.movement.x > map_size_x
       || aabb_intersection_terrain(&aabb, 0)) {
        camera.movement.x = 0.0F;
    }

    aabb_set_center(&aabb, camera.pos.x + camera.movement.x, camera.pos.y + camera.movement.y - camera.eye_height, camera.pos.z);
    if (camera.pos.y + camera.movement.y < 0 || aabb_intersection_terrain(&aabb, 0)) {
        camera.movement.y = 0.0F;
    }

    aabb_set_center(&aabb, camera.pos.x + camera.movement.x, camera.pos.y + camera.movement.y - camera.eye_height,
                    camera.pos.z + camera.movement.z);
    if (camera.pos.z + camera.movement.z < 0 || camera.pos.z + camera.movement.z > map_size_z
       || aabb_intersection_terrain(&aabb, 0)) {
        camera.movement.z = 0.0F;
    }

    if (cameracontroller_bodyview_mode) {
        // check if we cant spectate the player anymore
        for (int k = 0; k < PLAYERS_MAX * 2;
            k++) { // a while (1) loop caused it to get stuck on map change when playing on babel
            if (player_can_spectate(&players[cameracontroller_bodyview_player]))
                break;
            cameracontroller_bodyview_player = (cameracontroller_bodyview_player + 1) % PLAYERS_MAX;
        }
    }

    if (cameracontroller_bodyview_mode && players[cameracontroller_bodyview_player].alive) {
        Player * p    = &players[cameracontroller_bodyview_player];
        camera.pos    = p->physics.eye;
        camera.pos.y += player_height(p);
        camera.v      = p->physics.velocity;
    } else {
        camera.pos.x += camera.movement.x;
        camera.pos.y += camera.movement.y;
        camera.pos.z += camera.movement.z;
        camera.v      = camera.movement;
    }
}

void cameracontroller_spectator_render() {
    if (cameracontroller_bodyview_mode && players[cameracontroller_bodyview_player].alive) {
        Player * p = &players[cameracontroller_bodyview_player];

        Vector3f * o = settings.smooth_orientation ? &p->orientation_smooth : &p->orientation;

        float n = hypot3f(o->x, o->y, o->z);
        float ox = o->x / n, oy = o->y / n, oz = o->z / n;
        matrix_lookAt(matrix_view, camera.pos.x, camera.pos.y, camera.pos.z, camera.pos.x + ox, camera.pos.y + oy, camera.pos.z + oz, 0.0F,
                      1.0F, 0.0F);
    } else {
        matrix_lookAt(matrix_view, camera.pos.x, camera.pos.y, camera.pos.z, camera.pos.x + sin(camera.rot.x) * sin(camera.rot.y),
                      camera.pos.y + cos(camera.rot.y), camera.pos.z + cos(camera.rot.x) * sin(camera.rot.y), 0.0F, 1.0F, 0.0F);
    }
}

void cameracontroller_bodyview(float dt) {
    // check if we cant spectate the player anymore
    for (int k = 0; k < PLAYERS_MAX * 2;
        k++) { // a while (1) loop caused it to get stuck on map change when playing on babel
        if (player_can_spectate(&players[cameracontroller_bodyview_player]))
            break;
        cameracontroller_bodyview_player = (cameracontroller_bodyview_player + 1) % PLAYERS_MAX;
    }

    AABB aabb = {.min = {0, 0, 0}, .max = {0, 0, 0}};
    aabb_set_size(&aabb, 0.4F, 0.4F, 0.4F);

    float k;
    float traverse_lengths[2] = {-1, -1};
    for (k = 0.0F; k < 5.0F; k += 0.05F) {
        aabb_set_center(&aabb,
                        players[cameracontroller_bodyview_player].pos.x - sin(camera.rot.x) * sin(camera.rot.y) * k,
                        players[cameracontroller_bodyview_player].pos.y - cos(camera.rot.y) * k
                            + player_height2(&players[cameracontroller_bodyview_player]),
                        players[cameracontroller_bodyview_player].pos.z - cos(camera.rot.x) * sin(camera.rot.y) * k);

        if (aabb_intersection_terrain(&aabb, 0) && traverse_lengths[0] < 0)
            traverse_lengths[0] = fmax(k - 0.1F, 0);

        aabb_set_center(&aabb,
                        players[cameracontroller_bodyview_player].pos.x + sin(camera.rot.x) * sin(camera.rot.y) * k,
                        players[cameracontroller_bodyview_player].pos.y + cos(camera.rot.y) * k
                            + player_height2(&players[cameracontroller_bodyview_player]),
                        players[cameracontroller_bodyview_player].pos.z + cos(camera.rot.x) * sin(camera.rot.y) * k);

        if (!aabb_intersection_terrain(&aabb, 0) && traverse_lengths[1] < 0)
            traverse_lengths[1] = fmax(k - 0.1F, 0);
    }

    if (traverse_lengths[0] < 0) traverse_lengths[0] = 5.0F;
    if (traverse_lengths[1] < 0) traverse_lengths[1] = 5.0F;

    float tmp = (traverse_lengths[0] <= 0) ? (-traverse_lengths[1]) : traverse_lengths[0];

    cameracontroller_bodyview_zoom
        = (tmp < cameracontroller_bodyview_zoom) ? tmp : fmin(tmp, cameracontroller_bodyview_zoom + dt * 8.0F);

    // this is needed to determine which chunks need/can be rendered and for sound, minimap etc...
    camera.pos.x = players[cameracontroller_bodyview_player].pos.x
        - sin(camera.rot.x) * sin(camera.rot.y) * cameracontroller_bodyview_zoom;
    camera.pos.y = players[cameracontroller_bodyview_player].pos.y - cos(camera.rot.y) * cameracontroller_bodyview_zoom
        + player_height2(&players[cameracontroller_bodyview_player]);
    camera.pos.z = players[cameracontroller_bodyview_player].pos.z
        - cos(camera.rot.x) * sin(camera.rot.y) * cameracontroller_bodyview_zoom;

    camera.v = players[cameracontroller_bodyview_player].physics.velocity;

    if (cameracontroller_bodyview_mode && players[cameracontroller_bodyview_player].alive) {
        Player * p = &players[cameracontroller_bodyview_player];
        camera.pos    = p->physics.eye;
        camera.pos.y += player_height(p);
        camera.v      = p->physics.velocity;
    }
}

void cameracontroller_bodyview_render() {
    if (cameracontroller_bodyview_mode && players[cameracontroller_bodyview_player].alive) {
        Player * p = &players[cameracontroller_bodyview_player];
        float l  = hypot3f(p->orientation_smooth.x, p->orientation_smooth.y, p->orientation_smooth.z);
        float ox = p->orientation_smooth.x / l;
        float oy = p->orientation_smooth.y / l;
        float oz = p->orientation_smooth.z / l;

        matrix_lookAt(matrix_view, camera.pos.x, camera.pos.y, camera.pos.z, camera.pos.x + ox, camera.pos.y + oy, camera.pos.z + oz, 0.0F,
                      1.0F, 0.0F);
    } else {
        matrix_lookAt(matrix_view,
                      players[cameracontroller_bodyview_player].pos.x
                          - sin(camera.rot.x) * sin(camera.rot.y) * cameracontroller_bodyview_zoom,
                      players[cameracontroller_bodyview_player].pos.y
                          - cos(camera.rot.y) * cameracontroller_bodyview_zoom
                          + player_height2(&players[cameracontroller_bodyview_player]),
                      players[cameracontroller_bodyview_player].pos.z
                          - cos(camera.rot.x) * sin(camera.rot.y) * cameracontroller_bodyview_zoom,
                      players[cameracontroller_bodyview_player].pos.x,
                      players[cameracontroller_bodyview_player].pos.y
                          + player_height2(&players[cameracontroller_bodyview_player]),
                      players[cameracontroller_bodyview_player].pos.z, 0.0F, 1.0F, 0.0F);
    }
}

void cameracontroller_selection(float dt) {
    UNUSED(dt);

    camera.pos = (Vector3f) {256.0F, 79.0F, 256.0F};
    camera.v   = (Vector3f) {0.0F, 0.0F, 0.0F};

    matrix_rotate(matrix_view, 90.0F, 1.0F, 0.0F, 0.0F);
    matrix_translate(matrix_view, -camera.pos.x, -camera.pos.y, -camera.pos.z);
}

void cameracontroller_selection_render() {
    matrix_rotate(matrix_view, 90.0F, 1.0F, 0.0F, 0.0F);
    matrix_translate(matrix_view, -camera.pos.x, -camera.pos.y, -camera.pos.z);
}
