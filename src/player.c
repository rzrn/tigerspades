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

#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <BetterSpades/common.h>
#include <BetterSpades/camera.h>
#include <BetterSpades/player.h>
#include <BetterSpades/sound.h>
#include <BetterSpades/map.h>
#include <BetterSpades/matrix.h>
#include <BetterSpades/model.h>
#include <BetterSpades/font.h>
#include <BetterSpades/cameracontroller.h>
#include <BetterSpades/config.h>
#include <BetterSpades/tracer.h>
#include <BetterSpades/weapon.h>
#include <BetterSpades/window.h>
#include <BetterSpades/particle.h>
#include <BetterSpades/opengl.h>

GameState gamestate;

MouseButtons button_map;

LocalPlayer local_player = {
    .id                = 0,
    .health            = 100,
    .blocks            = 50,
    .grenades          = 3,
    .ammo              = 0,
    .ammo_reserved     = 0,
    .respawn_time      = 0,
    .respawn_cnt_last  = 255,
    .death_time        = 0.0F,
    .last_tool         = 0,
    .last_damage_timer = -INFINITY,
    .last_damage       = {0.0F, 0.0F, 0.0F},
    .drag_active       = false,
    .drag              = {0, 0, 0},
    .color             = {3, 0},
};

int default_team = -1, default_gun = -1;

int player_intersection_type = -1;
int player_intersection_player = 0;
float player_intersection_dist = 1024.0F;

Player players[PLAYERS_MAX];

#define FALL_DAMAGE_VELOCITY 0.58F
#define FALL_SLOW_DOWN       0.24F
#define SQRT                 0.70710678F
#define WEAPON_PRIMARY       1
#define FALL_DAMAGE_SCALAR   4096

void player_init() {
    for (int k = 0; k < PLAYERS_MAX; k++) {
        player_reset(&players[k]);
        players[k].score = 0;
    }
}

void player_reset(Player * p) {
    p->connected          = 0;
    p->alive              = 0;
    p->held_item          = TOOL_GUN;
    p->block              = Gray;
    p->physics.velocity.x = 0.0F;
    p->physics.velocity.y = 0.0F;
    p->physics.velocity.z = 0.0F;
    p->physics.jump       = 0;
    p->physics.airborne   = 0;
    p->physics.wade       = 0;
    p->input.keys         = 0;
    p->input.buttons      = 0;
}

void player_on_held_item_change(Player * p) {
    if (HASBIT(p->input.buttons, BUTTON_PRIMARY))
        p->start.lmb = window_time() + 0.8F;

    if (HASBIT(p->input.buttons, BUTTON_SECONDARY))
        p->start.rmb = window_time() + 0.8F;

    p->item_disabled    = window_time();
    p->items_show_start = window_time();
    p->item_showup      = window_time() + 0.3F;
    p->items_show       = 1;
}

int player_can_spectate(Player * p) {
    return p->connected
        && ((players[local_player.id].team != TEAM_SPECTATOR && p->team == players[local_player.id].team)
            || (players[local_player.id].team == TEAM_SPECTATOR && p->team != TEAM_SPECTATOR));
}

float player_swing_func(float x) {
    x -= (int) x;
    return (x < 0.5F) ? (x * 4.0F - 1.0F) : (3.0F - x * 4.0F);
}

float player_spade_func(float x) {
    return 1.0F - (x * 5 - (int)(x * 5));
}

float * player_tool_func(const Player * p) {
    static float ret[3];
    ret[0] = ret[1] = ret[2] = 0.0F;
    switch (p->held_item) {
        case TOOL_SPADE: {
            float t = window_time() - p->spade_use_timer;
            if (p->spade_use_type == 1 && t > 0.2F) return ret;
            if (p->spade_use_type == 2 && t > 1.0F) return ret;

            if (p == &players[local_player.id] && camera.mode == CAMERAMODE_FPS) {
                if (p->spade_use_type == 1) {
                    ret[0] = player_spade_func(t) * 90.0F;
                    return ret;
                }

                if (p->spade_use_type == 2) {
                    if (t <= 0.4F) {
                        ret[0] = 60.0F - player_spade_func(t / 2.0F) * 60.0F;
                        ret[1] = -t / 0.4F * 22.5F;
                        return ret;
                    }

                    if (t <= 0.7F) {
                        ret[0] = 60.0F;
                        ret[1] = -22.5F;
                        return ret;
                    }

                    if (t <= 1.0F) {
                        ret[0] = player_spade_func((t - 0.7F) / 5 / 0.3F) * 60.0F;
                        ret[1] = (t - 0.7F) / 0.4F * 22.5F - 22.5F;
                        return ret;
                    }
                }
            } else {
                if (HASBIT(p->input.buttons, BUTTON_PRIMARY)) {
                    ret[0] = (player_swing_func((window_time() - p->spade_use_timer) * 2.5F) + 1.0F) / 2.0F * 60.0F;
                    return ret;
                }

                if (HASBIT(p->input.buttons, BUTTON_SECONDARY)) {
                    ret[0] = (player_swing_func((window_time() - p->spade_use_timer) * 0.5F) + 1.0F) / 2.0F * 60.0F;
                    return ret;
                }
            }
        }
            // case TOOL_GRENADE:
            /*if (p->input.buttons.lmb && p!=&players[local_player.id]) {
                ret[0] = max(-(window_time()-p->input.buttons.lmb_start)*35.0F,-35.0F);
                return ret;
            } else {
                return ret;
            }*/
    }
    return ret;
}

float * player_tool_translate_func(Player * p) {
    static float ret[3];
    ret[0] = ret[1] = ret[2] = 0.0F;
    if (p == &players[local_player.id] && camera.mode == CAMERAMODE_FPS) {
        if (window_time() - p->item_showup < 0.5F) {
            return ret;
        }
        if (p->held_item == TOOL_GUN
           && window_time() - weapon_last_shot < weapon_delay(players[local_player.id].weapon)) {
            ret[2] = -(weapon_delay(players[local_player.id].weapon) - (window_time() - weapon_last_shot))
                / weapon_delay(players[local_player.id].weapon) * weapon_recoil_anim(players[local_player.id].weapon)
                * (local_player.ammo > 0);
            return ret;
        }

        if (p->held_item == TOOL_SPADE) {
            float t = window_time() - p->spade_use_timer;
            if (t > 1.0F) {
                return ret;
            }
            if (p->spade_use_type == 2) {
                if (t > 0.4F && t <= 0.7F) {
                    ret[2] = (t - 0.4F) / 0.3F * 0.8F;
                    return ret;
                }
                if (t > 0.7F) {
                    ret[2] = (0.3F - (t - 0.7F)) / 0.3F * 0.8F;
                    return ret;
                }
            }
        }
        if (p->held_item == TOOL_GRENADE) {
            if (HASBIT(p->input.buttons, BUTTON_PRIMARY)) {
                ret[1] = (window_time() - p->start.lmb) * 1.3F;
                ret[0] = -ret[1];
                return ret;
            } else {
                return ret;
            }
        }
    }
    return ret;
}

float player_height(const Player * p) {
    return HASBIT(p->input.keys, INPUT_CROUCH) ? 1.05F : 1.1F;
}

float player_height2(const Player * p) {
    return p->alive ? 0.0F : 1.0F;
}

float player_section_height(int section) {
    switch (section) {
        case HITTYPE_HEAD:  return +1.00F;
        case HITTYPE_TORSO: return +0.00F;
        case HITTYPE_ARMS:  return +0.25F;
        case HITTYPE_LEGS:  return -1.00F;
        default:            return +0.00F;
    }
}

bool player_intersection_exists(Hit * s) {
    return s->head || s->torso || s->arms || s->leg_left || s->leg_right;
}

int player_intersection_choose(Hit * s, float * dist) {
    int type;
    *dist = FLT_MAX;

    if (s->arms && s->distance.arms < *dist) {
        type = HITTYPE_ARMS;
        *dist = s->distance.arms;
    }

    if (s->leg_left && s->distance.leg_left < *dist) {
        type = HITTYPE_LEGS;
        *dist = s->distance.leg_left;
    }

    if (s->leg_right && s->distance.leg_right < *dist) {
        type = HITTYPE_LEGS;
        *dist = s->distance.leg_right;
    }

    if (s->torso && s->distance.torso < *dist) {
        type = HITTYPE_TORSO;
        *dist = s->distance.torso;
    }

    if (s->head && s->distance.head < *dist) {
        type = HITTYPE_HEAD;
        *dist = s->distance.head;
    }

    return type;
}

void player_update(float dt, int locked) {
    for (int k = 0; k < PLAYERS_MAX; k++) {
        if (players[k].connected) {
            if (locked) {
                player_move(&players[k], dt, k);
            } else {
                if (k != local_player.id) {
                    // smooth out player orientation
                    players[k].orientation_smooth.x = players[k].orientation_smooth.x * pow(0.9F, dt * 60.0F)
                        + players[k].orientation.x * pow(0.1F, dt * 60.0F);
                    players[k].orientation_smooth.y = players[k].orientation_smooth.y * pow(0.9F, dt * 60.0F)
                        + players[k].orientation.y * pow(0.1F, dt * 60.0F);
                    players[k].orientation_smooth.z = players[k].orientation_smooth.z * pow(0.9F, dt * 60.0F)
                        + players[k].orientation.z * pow(0.1F, dt * 60.0F);
                }
            }
        }
    }
}

void player_render_all() {
    player_intersection_type = -1;
    player_intersection_dist = FLT_MAX;

    Ray ray;
    ray.origin[X] = camera.pos.x;
    ray.origin[Y] = camera.pos.y;
    ray.origin[Z] = camera.pos.z;

    ray.direction[X] = sin(camera.rot.x) * sin(camera.rot.y);
    ray.direction[Y] = cos(camera.rot.y);
    ray.direction[Z] = cos(camera.rot.x) * sin(camera.rot.y);

    for (int k = 0; k < PLAYERS_MAX; k++) {
        if (!players[k].connected || players[k].team == TEAM_SPECTATOR)
            continue;

        if (!HASBIT(players[k].input.buttons, BUTTON_PRIMARY) &&
            !HASBIT(players[k].input.buttons, BUTTON_SECONDARY)) {
            players[k].spade_used = 0;

            if (players[k].spade_use_type == 1)
                players[k].spade_use_type = 0;

            if (players[k].spade_use_type == 2)
                players[k].spade_use_timer = 0;
        }

        if (players[k].alive && players[k].held_item == TOOL_SPADE
           && (HASBIT(players[k].input.buttons, BUTTON_PRIMARY) ||
               HASBIT(players[k].input.buttons, BUTTON_SECONDARY))
           && window_time() - players[k].item_showup >= 0.5F) {
            // now run a hitscan and see if any block or player is in the way
            CameraHit hit;
            if (HASBIT(players[k].input.buttons, BUTTON_PRIMARY) &&
               (window_time() - players[k].spade_use_timer > 0.2F)) {
                camera_hit_fromplayer(&hit, k, 4.0F);
                if (hit.y == 0 && hit.type == CAMERA_HITTYPE_BLOCK)
                    hit.type = CAMERA_HITTYPE_NONE;

                switch (hit.type) {
                    case CAMERA_HITTYPE_BLOCK: {
                        sound_create(SOUND_WORLD, sound(SOUND_HITGROUND), hit.x + 0.5F, hit.y + 0.5F, hit.z + 0.5F);

                        if (k == local_player.id)
                            map_damage(hit.x, hit.y, hit.z, 50);

                        if (k == local_player.id && map_damage_action(hit.x, hit.y, hit.z) && isdestructible(hit.x, hit.y, hit.z)) {
                            PacketBlockAction contained;
                            contained.action_type = ACTION_DESTROY;
                            contained.player_id   = local_player.id;
                            contained.pos.x       = hit.x;
                            contained.pos.y       = hit.z;
                            contained.pos.z       = 63 - hit.y;

                            doPacketBlockAction(&contained);
                            local_player.blocks = min(local_player.blocks + 1, 50);
                        } else {
                            particle_create(map_get(hit.x, hit.y, hit.z), hit.xb + 0.5F, hit.yb + 0.5F, hit.zb + 0.5F,
                                            2.5F, 1.0F, 4, 0.1F, 0.25F);
                        }
                        break;
                    }

                    case CAMERA_HITTYPE_PLAYER: {
                        sound_create_sticky(sound(SOUND_SPADE_WHACK), players + k, k);
                        particle_create(
                            Red,
                            players[hit.player_id].physics.eye.x,
                            players[hit.player_id].physics.eye.y + player_section_height(hit.player_section),
                            players[hit.player_id].physics.eye.z,
                            3.5F, 1.0F, 8, 0.1F, 0.4F
                        );

                        if (k == local_player.id) {
                            PacketHit contained;
                            contained.player_id = hit.player_id;
                            contained.hit_type  = HITTYPE_SPADE;
                            sendPacketHit(&contained, 0);
                        }
                        break;
                    }

                    case CAMERA_HITTYPE_NONE: sound_create_sticky(sound(SOUND_SPADE_WOOSH), players + k, k); break;
                }

                players[k].spade_use_type  = 1;
                players[k].spade_used      = 1;
                players[k].spade_use_timer = window_time();
            }

            if (HASBIT(players[k].input.buttons, BUTTON_SECONDARY) &&
               (window_time() - players[k].spade_use_timer > 1.0F)) {
                if (players[k].spade_used) {
                    camera_hit_fromplayer(&hit, k, 4.0F);

                    if (hit.type == CAMERA_HITTYPE_BLOCK && isdestructible(hit.x, hit.y, hit.z)) {
                        sound_create(SOUND_WORLD, sound(SOUND_HITGROUND), hit.x + 0.5F, hit.y + 0.5F, hit.z + 0.5F);

                        if (k == local_player.id) {
                            PacketBlockAction contained;
                            contained.action_type = ACTION_SPADE;
                            contained.player_id   = local_player.id;
                            contained.pos.x       = hit.x;
                            contained.pos.y       = hit.z;
                            contained.pos.z       = 63 - hit.y;

                            doPacketBlockAction(&contained);
                        }
                    } else {
                        sound_create_sticky(sound(SOUND_SPADE_WOOSH), players + k, k);
                    }
                }

                players[k].spade_use_type  = 2;
                players[k].spade_used      = 1;
                players[k].spade_use_timer = window_time();
            }
        }

        if (k != local_player.id) {
            if (camera_CubeInFrustum(players[k].pos.x, players[k].pos.y, players[k].pos.z, 1.0F, 2.0F)
               && norm2f(players[k].pos.x, players[k].pos.z, camera.pos.x, camera.pos.z) <=
                  sqrf(settings.render_distance + 2.0F)) {
                Hit intersects = {0};
                player_render(players + k, k);
                player_collision(players + k, &ray, &intersects);
                if (player_intersection_exists(&intersects)) {
                    float d;
                    int type = player_intersection_choose(&intersects, &d);
                    if (d < player_intersection_dist) {
                        player_intersection_dist = d;
                        player_intersection_player = k;
                        player_intersection_type = type;
                    }
                }
            }

            if (players[k].alive && players[k].held_item == TOOL_GUN && HASBIT(players[k].input.buttons, BUTTON_PRIMARY)) {
                if (window_time() - players[k].gun_shoot_timer > weapon_delay(players[k].weapon) && players[k].ammo > 0) {
                    players[k].ammo--;
                    sound_create_sticky(weapon_sound(players[k].weapon), players + k, k);

                    float o[3] = {players[k].orientation.x, players[k].orientation.y, players[k].orientation.z};

                    weapon_spread(&players[k], o);

                    CameraHit hit;
                    camera_hit(&hit, k, players[k].physics.eye.x, players[k].physics.eye.y + player_height(&players[k]),
                               players[k].physics.eye.z, o[0], o[1], o[2], 128.0F);
                    tracer_pvelocity(o, &players[k]);
                    tracer_add(players[k].weapon, players[k].physics.eye.x,
                               players[k].physics.eye.y + player_height(&players[k]), players[k].physics.eye.z, o[0],
                               o[1], o[2]);
                    particle_create_casing(&players[k]);

                    if (!network_connected || local_hit_effects) switch (hit.type) {
                        case CAMERA_HITTYPE_PLAYER: {
                            sound_create_sticky(
                                sound(hit.player_section == HITTYPE_HEAD ? SOUND_SPADE_WHACK : SOUND_HITPLAYER),
                                players + hit.player_id, hit.player_id
                            );

                            particle_create(
                                Red,
                                players[hit.player_id].physics.eye.x,
                                players[hit.player_id].physics.eye.y + player_section_height(hit.player_section),
                                players[hit.player_id].physics.eye.z,
                                3.5F, 1.0F, 8, 0.1F, 0.4F
                            );
                            break;
                        }

                        case CAMERA_HITTYPE_BLOCK: {
                            particle_create(map_get(hit.x, hit.y, hit.z), hit.xb + 0.5F, hit.yb + 0.5F, hit.zb + 0.5F,
                                            2.5F, 1.0F, 4, 0.1F, 0.25F);
                            break;
                        }
                    }

                    players[k].gun_shoot_timer = window_time();
                }
            }
        }
    }
}

static float foot_function(const Player * p) {
    float f = (window_time() - p->sound.feet_started_cycle) /
              (HASBIT(p->input.keys, INPUT_SPRINT) ? (0.5F / 1.3F) : 0.5F);
    f = f * 2.0F - 1.0F;
    return p->sound.feet_cylce ? f : -f;
}

typedef struct {
    float pivot[3];
    int size[3];
    float scale;
} Hitbox;

static const Hitbox box_head = {
    .pivot = {2.5F, 2.5F, 0.5F},
    .size  = {6, 6, 6},
    .scale = 0.1F,
};

static const Hitbox box_torso = {
    .pivot = {3.5F, 1.5F, 9.5F},
    .size  = {8, 4, 9},
    .scale = 0.1F,
};

static const Hitbox box_torsoc = {
    .pivot = {3.5F, 6.5F, 6.5F},
    .size  = {8, 8, 7},
    .scale = 0.1F,
};

static const Hitbox box_arm_left = {
    .pivot = {5.5F, -0.5F, 5.5F},
    .size  = {2, 9, 6},
    .scale = 0.1F,
};

static const Hitbox box_arm_right = {
    .pivot = {-3.5F, 4.25F, 1.5F},
    .size  = {3, 14, 2},
    .scale = 0.1F,
};

static const Hitbox box_leg = {
    .pivot = {1.0F, 1.5F, 12.0F},
    .size  = {3, 5, 12},
    .scale = 0.1F,
};

static const Hitbox box_legc = {
    .pivot = {1.0F, 1.5F, 7.0F},
    .size  = {3, 7, 8},
    .scale = 0.1F,
};

static bool hitbox_intersection(mat4 model, const Hitbox * box, Ray * r, float * distance) {
    mat4 inv_model;
    glm_mat4_inv(model, inv_model);

    vec3 origin, dir;
    glm_mat4_mulv3(inv_model, r->origin, 1.0F, origin);
    glm_mat4_mulv3(inv_model, r->direction, 0.0F, dir);

    return aabb_intersection_ray(
        &(AABB) {
            .min = {-box->pivot[0] * box->scale, -box->pivot[2] * box->scale, -box->pivot[1] * box->scale},
            .max = {(box->size[0] - box->pivot[0]) * box->scale, (box->size[2] - box->pivot[2]) * box->scale,
                    (box->size[1] - box->pivot[1]) * box->scale},
        },
        &(Ray) {
            .origin = {origin[0], origin[1], origin[2]},
            .direction = {dir[0], dir[1], dir[2]},
        },
        distance
    );
}

void player_collision(const Player * p, Ray * ray, Hit * intersects) {
    if (!p->alive || p->team == TEAM_SPECTATOR)
        return;

    float l  = hypot3f(p->orientation_smooth.x, p->orientation_smooth.y, p->orientation_smooth.z);
    float ox = p->orientation_smooth.x / l;
    float oy = p->orientation_smooth.y / l;
    float oz = p->orientation_smooth.z / l;

    const Hitbox * torso = HASBIT(p->input.keys, INPUT_CROUCH) ? &box_torsoc : &box_torso;
    const Hitbox * leg   = HASBIT(p->input.keys, INPUT_CROUCH) ? &box_legc   : &box_leg;

    float height = player_height(p) - 0.25F;

    float len = hypot2f(p->orientation.x, p->orientation.z);
    float fx  = p->orientation.x / len;
    float fy  = p->orientation.z / len;

    float a = (p->physics.velocity.x * fx + fy * p->physics.velocity.z) / (fx * fx + fy * fy);
    float b = (p->physics.velocity.z - fy * a) / fx;
    a /= 0.25F;
    b /= 0.25F;

    float dist; // distance

    matrix_push(matrix_model);

    matrix_identity(matrix_model);
    matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
    float head_scale = hypot3f(p->orientation.x, p->orientation.y, p->orientation.z);
    matrix_translate(matrix_model, 0.0F, box_head.pivot[2] * (head_scale * box_head.scale - box_head.scale), 0.0F);
    matrix_scale3(matrix_model, head_scale);
    matrix_pointAt(matrix_model, ox, oy, oz);
    matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);

    if (hitbox_intersection(matrix_model, &box_head, ray, &dist)) {
        intersects->head          = 1;
        intersects->distance.head = dist;
    }

    matrix_identity(matrix_model);
    matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
    matrix_pointAt(matrix_model, ox, 0.0F, oz);
    matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);

    if (hitbox_intersection(matrix_model, torso, ray, &dist)) {
        intersects->torso          = 1;
        intersects->distance.torso = dist;
    }

    matrix_push(matrix_model);
    matrix_translate(matrix_model, torso->size[0] * 0.1F * 0.5F - leg->size[0] * 0.1F * 0.5F,
                     -torso->size[2] * 0.1F * (HASBIT(p->input.keys, INPUT_CROUCH) ? 0.6F : 1.0F),
                     HASBIT(p->input.keys, INPUT_CROUCH) ? (-torso->size[2] * 0.1F * 0.75F) : 0.0F);
    matrix_rotate(matrix_model, 45.0F * foot_function(p) * a, 1.0F, 0.0F, 0.0F);
    matrix_rotate(matrix_model, 45.0F * foot_function(p) * b, 0.0F, 0.0F, 1.0F);

    if (hitbox_intersection(matrix_model, leg, ray, &dist)) {
        intersects->leg_left          = 1;
        intersects->distance.leg_left = dist;
    }

    matrix_pop(matrix_model);
    matrix_translate(matrix_model, -torso->size[0] * 0.1F * 0.5F + leg->size[0] * 0.1F * 0.5F,
                     -torso->size[2] * 0.1F * (HASBIT(p->input.keys, INPUT_CROUCH) ? 0.6F : 1.0F),
                     HASBIT(p->input.keys, INPUT_CROUCH) ? (-torso->size[2] * 0.1F * 0.75F) : 0.0F);
    matrix_rotate(matrix_model, -45.0F * foot_function(p) * a, 1.0F, 0.0F, 0.0F);
    matrix_rotate(matrix_model, -45.0F * foot_function(p) * b, 0.0F, 0.0F, 1.0F);

    if (hitbox_intersection(matrix_model, leg, ray, &dist)) {
        intersects->leg_right          = 1;
        intersects->distance.leg_right = dist;
    }

    matrix_identity(matrix_model);
    matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
    matrix_translate(matrix_model, 0.0F, (HASBIT(p->input.keys, INPUT_CROUCH) ? 0.1F : 0.0F) - 0.1F * 2, 0.0F);
    matrix_pointAt(matrix_model, ox, oy, oz);
    matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);

    if (HASBIT(p->input.keys, INPUT_SPRINT) && !HASBIT(p->input.keys, INPUT_CROUCH))
        matrix_rotate(matrix_model, 45.0F, 1.0F, 0.0F, 0.0F);

    float * angles = player_tool_func(p);
    matrix_rotate(matrix_model, angles[0], 1.0F, 0.0F, 0.0F);
    matrix_rotate(matrix_model, angles[1], 0.0F, 1.0F, 0.0F);

    if (hitbox_intersection(matrix_model, &box_arm_left, ray, &dist)) {
        intersects->arms          = 1;
        intersects->distance.arms = dist;
    }

    matrix_rotate(matrix_model, -45.0F, 0.0F, 1.0F, 0.0F);

    if (hitbox_intersection(matrix_model, &box_arm_right, ray, &dist)) {
        intersects->arms          = 1;
        intersects->distance.arms = dist;
    }

    matrix_pop(matrix_model);
}

void player_render(Player * p, int id) {
    kv6_calclight(p->pos.x, p->pos.y, p->pos.z);

#if HACKS_ENABLED && HACK_ESP
    if (id != local_player.id)
#else
    if (camera.mode == CAMERAMODE_SPECTATOR && p->team != TEAM_SPECTATOR && !cameracontroller_bodyview_mode)
#endif
    {
        matrix_push(matrix_model);
        matrix_translate(matrix_model, p->pos.x, p->physics.eye.y + player_height(p) + 1.25F, p->pos.z);
        matrix_rotate(matrix_model, camera.rot.x / PI * 180.0F + 180.0F, 0.0F, 1.0F, 0.0F);
        matrix_rotate(matrix_model, -camera.rot.y / PI * 180.0F + 90.0F, 1.0F, 0.0F, 0.0F);
        matrix_scale(matrix_model, 1.0F / 92.0F, 1.0F / 92.0F, 1.0F / 92.0F);
        matrix_upload();

        switch (p->team) {
            case TEAM1: glColorRGB3i(gamestate.team1.color); break;
            case TEAM2: glColorRGB3i(gamestate.team2.color); break;
        }

        font_select(FONT_FIXEDSYS);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5F);
        glDisable(GL_DEPTH_TEST);
        font_centered(0, 0, 4, p->name, UTF8);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_ALPHA_TEST);
        matrix_pop(matrix_model);
        matrix_upload();
    }

    float l  = hypot3f(p->orientation_smooth.x, p->orientation_smooth.y, p->orientation_smooth.z);
    float ox = p->orientation_smooth.x / l;
    float oy = p->orientation_smooth.y / l;
    float oz = p->orientation_smooth.z / l;

    if (!p->alive) {
        if (id != local_player.id || camera.mode != CAMERAMODE_DEATH) {
            matrix_push(matrix_model);
            matrix_translate(matrix_model, p->pos.x, p->pos.y + 0.25F, p->pos.z);
            matrix_pointAt(matrix_model, ox, 0.0F, oz);
            matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
            if (p->physics.velocity.y < 0.05F && p->pos.y < 1.5F)
                matrix_translate(matrix_model, 0.0F, (sin(window_time() * 1.5F) - 1.0F) * 0.1F, 0.0F);
            matrix_upload();
            kv6_render(&model[MODEL_PLAYERDEAD], p->team);
            matrix_pop(matrix_model);
        }

        return;
    }

    float time = window_time() * 1000.0F;

    kv6 * torso  = &model[HASBIT(p->input.keys, INPUT_CROUCH) ? MODEL_PLAYERTORSOC : MODEL_PLAYERTORSO];
    kv6 * leg    = &model[HASBIT(p->input.keys, INPUT_CROUCH) ? MODEL_PLAYERLEGC   : MODEL_PLAYERLEG];
    float height = player_height(p);

    if (id != local_player.id)
        height -= 0.25F;

    int render_body = (id != local_player.id || !p->alive || camera.mode != CAMERAMODE_FPS)
        && !((camera.mode == CAMERAMODE_BODYVIEW || camera.mode == CAMERAMODE_SPECTATOR)
             && cameracontroller_bodyview_mode && cameracontroller_bodyview_player == id);
    int render_fpv = (id == local_player.id && camera.mode == CAMERAMODE_FPS)
        || ((camera.mode == CAMERAMODE_BODYVIEW || camera.mode == CAMERAMODE_SPECTATOR)
            && cameracontroller_bodyview_mode && cameracontroller_bodyview_player == id);

    if (render_body) {
        static kv6 * const model_playerhead = &model[MODEL_PLAYERHEAD];

        matrix_push(matrix_model);
        matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
        float head_scale = hypot3f(p->orientation.x, p->orientation.y, p->orientation.z);
        matrix_translate(matrix_model, 0.0F, model_playerhead->zpiv * (head_scale * model_playerhead->scale - model_playerhead->scale), 0.0F);
        matrix_scale3(matrix_model, head_scale);
        matrix_pointAt(matrix_model, ox, oy, oz);
        matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
        matrix_upload();
        kv6_render(model_playerhead, p->team);
        matrix_pop(matrix_model);
    }

    if (render_body) {
        matrix_push(matrix_model);
        matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
        matrix_pointAt(matrix_model, ox, 0.0F, oz);
        matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
        matrix_upload();
        kv6_render(torso, p->team);
        matrix_pop(matrix_model);

        if (gamestate.mode == GAMEMODE_CTF &&
            ((gamestate.ctf.team2_has_intel && gamestate.ctf.team1_carrier == id) ||
             (gamestate.ctf.team1_has_intel && gamestate.ctf.team2_carrier == id))) {
            static kv6 * const model_intel = &model[MODEL_INTEL];

            matrix_push(matrix_model);
            matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
            matrix_pointAt(matrix_model, -oz, 0.0F, ox);
            matrix_translate(
                matrix_model, (torso->xsiz - model_intel->xsiz) * 0.5F * torso->scale,
                -(torso->zpiv - torso->zsiz * 0.5F + model_intel->zsiz * (HASBIT(p->input.keys, INPUT_CROUCH) ? 0.125F : 0.25F)) * torso->scale,
                (torso->ypiv + model_intel->ypiv) * torso->scale
            );

            matrix_scale3(matrix_model, torso->scale / model_intel->scale);

            if (HASBIT(p->input.keys, INPUT_CROUCH))
                matrix_rotate(matrix_model, -45.0F, 1.0F, 0.0F, 0.0F);
            matrix_upload();

            int t = TEAM_SPECTATOR;

            if (gamestate.ctf.team2_has_intel && gamestate.ctf.team1_carrier == id)
                t = TEAM1;

            if (gamestate.ctf.team1_has_intel && gamestate.ctf.team2_carrier == id)
                t = TEAM2;

            kv6_render(model_intel, t);
            matrix_pop(matrix_model);
        }

        float len = hypot2f(p->orientation.x, p->orientation.z);
        float fx  = p->orientation.x / len;
        float fy  = p->orientation.z / len;

        float a = p->physics.velocity.x * fx + fy * p->physics.velocity.z;
        float b = (p->physics.velocity.z - fy * a) / fx;

        a /= 0.25F; b /= 0.25F;

        matrix_push(matrix_model);
        matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
        matrix_pointAt(matrix_model, ox, 0.0F, oz);
        matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
        matrix_translate(matrix_model, torso->xsiz * 0.1F * 0.5F - leg->xsiz * 0.1F * 0.5F,
                         -torso->zsiz * 0.1F * (HASBIT(p->input.keys, INPUT_CROUCH) ? 0.6F : 1.0F),
                         HASBIT(p->input.keys, INPUT_CROUCH) ? (-torso->zsiz * 0.1F * 0.75F) : 0.0F);
        matrix_rotate(matrix_model, 45.0F * foot_function(p) * a, 1.0F, 0.0F, 0.0F);
        matrix_rotate(matrix_model, 45.0F * foot_function(p) * b, 0.0F, 0.0F, 1.0F);
        matrix_upload();
        kv6_render(leg, p->team);
        matrix_pop(matrix_model);

        matrix_push(matrix_model);
        matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
        matrix_pointAt(matrix_model, ox, 0.0F, oz);
        matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
        matrix_translate(matrix_model, -torso->xsiz * 0.1F * 0.5F + leg->xsiz * 0.1F * 0.5F,
                         -torso->zsiz * 0.1F * (HASBIT(p->input.keys, INPUT_CROUCH) ? 0.6F : 1.0F),
                         HASBIT(p->input.keys, INPUT_CROUCH) ? (-torso->zsiz * 0.1F * 0.75F) : 0.0F);
        matrix_rotate(matrix_model, -45.0F * foot_function(p) * a, 1.0F, 0.0F, 0.0F);
        matrix_rotate(matrix_model, -45.0F * foot_function(p) * b, 0.0F, 0.0F, 1.0F);
        matrix_upload();
        kv6_render(leg, p->team);
        matrix_pop(matrix_model);
    }

    matrix_push(matrix_model);
    matrix_translate(matrix_model, p->physics.eye.x, p->physics.eye.y + height, p->physics.eye.z);
    if (!render_fpv) matrix_translate(matrix_model, 0.0F, (HASBIT(p->input.keys, INPUT_CROUCH) ? 0.1F : 0.0F) - 0.1F * 2, 0.0F);

    matrix_pointAt(matrix_model, ox, oy, oz);
    matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
    if (render_fpv)
        matrix_translate(matrix_model, 0.0F, -2 * 0.1F, -2 * 0.1F);

    if (render_fpv && p->alive) {
        float speed = hypot2f(p->physics.velocity.x, p->physics.velocity.z) / 0.25F;
        float * f = player_tool_translate_func(p);
        matrix_translate(matrix_model, f[X], f[Y], 0.1F * player_swing_func(time / 1000.0F) * speed + f[Z]);
    }

    if (HASBIT(p->input.keys, INPUT_SPRINT) && !HASBIT(p->input.keys, INPUT_CROUCH))
        matrix_rotate(matrix_model, 45.0F, 1.0F, 0.0F, 0.0F);

    if (render_fpv && window_time() - p->item_showup < 0.5F)
        matrix_rotate(matrix_model, 45.0F - (window_time() - p->item_showup) * 90.0F, 1.0F, 0.0F, 0.0F);

    if (!(p->held_item == TOOL_SPADE && render_fpv && camera.mode == CAMERAMODE_FPS)) {
        float * angles = player_tool_func(p);
        matrix_rotate(matrix_model, angles[0], 1.0F, 0.0F, 0.0F);
        matrix_rotate(matrix_model, angles[1], 0.0F, 1.0F, 0.0F);
    }

    if (settings.left_handed && id == local_player.id) {
        matrix_scale(matrix_model, -1.0F, 1.0F, 1.0F);
        glCullFace(GL_FRONT);
    }

    if (render_body || settings.player_arms) {
        matrix_upload();
        kv6_render(&model[MODEL_PLAYERARMS], p->team);
    }

    static kv6 * const model_spade = &model[MODEL_SPADE];

    matrix_translate(matrix_model, -3.5F * 0.1F + 0.01F, 0.0F, 10 * 0.1F);
    if (p->held_item == TOOL_SPADE && render_fpv && window_time() - p->item_showup >= 0.5F) {
        float * angles = player_tool_func(p);
        matrix_translate(matrix_model, 0.0F, (model_spade->zpiv - model_spade->zsiz) * 0.05F, 0.0F);
        matrix_rotate(matrix_model, angles[0], 1.0F, 0.0F, 0.0F);
        matrix_rotate(matrix_model, angles[1], 0.0F, 1.0F, 0.0F);
        matrix_translate(matrix_model, 0.0F, -(model_spade->zpiv - model_spade->zsiz) * 0.05F, 0.0F);
    }

    matrix_upload();
    switch (p->held_item) {
        case TOOL_SPADE: kv6_render(model_spade, p->team); break;

        case TOOL_BLOCK: {
            static kv6 * const model_block = &model[MODEL_BLOCK];

            model_block->red   = p->block.r / 255.0F;
            model_block->green = p->block.g / 255.0F;
            model_block->blue  = p->block.b / 255.0F;
            kv6_render(model_block, p->team);
            break;
        }

        case TOOL_GUN: {
            // matrix_translate(matrix_model, 3.0F*0.1F-0.01F+0.025F,0.25F,-0.0625F);
            // matrix_upload();
            if (!(render_fpv && HASBIT(p->input.buttons, BUTTON_SECONDARY)))
                kv6_render(weapon_model(p->weapon), p->team);

            break;
        }

        case TOOL_GRENADE: kv6_render(&model[MODEL_GRENADE], p->team); break;
    }

    vec4 v = {0.1F, 0, -0.3F, 1};
    matrix_vector(matrix_model, v);
    vec4 v2 = {1.1F, 0, -0.3F, 1};
    matrix_vector(matrix_model, v2);

    p->gun_pos.x = v[X];
    p->gun_pos.y = v[Y];
    p->gun_pos.z = v[Z];

    p->casing_dir.x = v[X] - v2[X];
    p->casing_dir.y = v[Y] - v2[Y];
    p->casing_dir.z = v[Z] - v2[Z];

    matrix_pop(matrix_model);

    glCullFace(GL_BACK);
}

int player_clipbox(float x, float y, float z) {
    int sz;

    if (x < 0 || x >= 512 || y < 0 || y >= 512)
        return 1;
    else if (z < 0)
        return 0;
    sz = (int) z;
    if (sz == 63)
        sz = 62;
    else if (sz >= 64)
        return 1;
    return !map_isair((int) x, 63 - sz, (int) y);
}

void player_reposition(Player * p) {
    p->physics.eye.x = p->pos.x;
    p->physics.eye.y = p->pos.y;
    p->physics.eye.z = p->pos.z;

    float f = p->physics.lastclimb - window_time();
    if (f > -0.25F && !HASBIT(p->input.keys, INPUT_CROUCH)) {
        p->physics.eye.z += (f + 0.25F) / 0.25F;
        if (&players[local_player.id] == p) {
            last_cy = 63.0F - p->physics.eye.z;
        }
    }
}

void player_coordsystem_adjust1(Player * p) {
    float tmp;

    tmp = p->pos.z;
    p->pos.z = 63.0F - p->pos.y;
    p->pos.y = tmp;

    tmp = p->physics.eye.z;
    p->physics.eye.z = 63.0F - p->physics.eye.y;
    p->physics.eye.y = tmp;

    tmp = p->physics.velocity.z;
    p->physics.velocity.z = -p->physics.velocity.y;
    p->physics.velocity.y = tmp;

    tmp = p->orientation.z;
    p->orientation.z = -p->orientation.y;
    p->orientation.y = tmp;
}

void player_coordsystem_adjust2(Player * p) {
    float tmp;

    tmp = p->pos.y;
    p->pos.y = 63.0F - p->pos.z;
    p->pos.z = tmp;

    tmp = p->physics.eye.y;
    p->physics.eye.y = 63.0F - p->physics.eye.z;
    p->physics.eye.z = tmp;

    tmp = p->physics.velocity.y;
    p->physics.velocity.y = -p->physics.velocity.z;
    p->physics.velocity.z = tmp;

    tmp = p->orientation.y;
    p->orientation.y = -p->orientation.z;
    p->orientation.z = tmp;
}

void player_boxclipmove(Player * p, float fsynctics) {
    float offset, m, f, nx, ny, nz, z;
    long climb = 0;

    f = fsynctics * 32.f;
    nx = f * p->physics.velocity.x + p->pos.x;
    ny = f * p->physics.velocity.y + p->pos.y;

    if (HASBIT(p->input.keys, INPUT_CROUCH)) {
        offset = 0.45f;
        m      = 0.9f;
    } else {
        offset = 0.9f;
        m      = 1.35f;
    }

    nz = p->pos.z + offset;

    if (p->physics.velocity.x < 0)
        f = -0.45f;
    else
        f = 0.45f;
    z = m;
    while (z >= -1.36f && !player_clipbox(nx + f, p->pos.y - 0.45f, nz + z)
          && !player_clipbox(nx + f, p->pos.y + 0.45f, nz + z))
        z -= 0.9f;
#if !(HACKS_ENABLED && HACK_NOCLIP)
    if (z < -1.36f)
#else
    if (true)
#endif
        p->pos.x = nx;
    else if (!HASBIT(p->input.keys, INPUT_CROUCH) &&
              (p->orientation.z < 0.5f)           &&
             !HASBIT(p->input.keys, INPUT_SPRINT)) {
        z = 0.35f;
        while (z >= -2.36f && !player_clipbox(nx + f, p->pos.y - 0.45f, nz + z)
              && !player_clipbox(nx + f, p->pos.y + 0.45f, nz + z))
            z -= 0.9f;
        if (z < -2.36f) {
            p->pos.x = nx;
            climb = 1;
        } else
            p->physics.velocity.x = 0;
    } else
        p->physics.velocity.x = 0;

    if (p->physics.velocity.y < 0)
        f = -0.45f;
    else
        f = 0.45f;
    z = m;
    while (z >= -1.36f && !player_clipbox(p->pos.x - 0.45f, ny + f, nz + z)
          && !player_clipbox(p->pos.x + 0.45f, ny + f, nz + z))
        z -= 0.9f;
#if !(HACKS_ENABLED && HACK_NOCLIP)
    if (z < -1.36f)
#else
    if (true)
#endif
        p->pos.y = ny;
    else if (!HASBIT(p->input.keys, INPUT_CROUCH) &&
              (p->orientation.z < 0.5f)           &&
             !HASBIT(p->input.keys, INPUT_SPRINT) &&
             !climb) {
        z = 0.35f;
        while (z >= -2.36f && !player_clipbox(p->pos.x - 0.45f, ny + f, nz + z)
              && !player_clipbox(p->pos.x + 0.45f, ny + f, nz + z))
            z -= 0.9f;
        if (z < -2.36f) {
            p->pos.y = ny;
            climb = 1;
        } else
            p->physics.velocity.y = 0;
    } else if (!climb)
        p->physics.velocity.y = 0;

    if (climb) {
        p->physics.velocity.x *= 0.5f;
        p->physics.velocity.y *= 0.5f;
        p->physics.lastclimb = window_time();
        nz--;
        m = -1.35f;
    } else {
        if (p->physics.velocity.z < 0)
            m = -m;
        nz += p->physics.velocity.z * fsynctics * 32.f;
    }

    p->physics.airborne = 1;

    if (player_clipbox(p->pos.x - 0.45f, p->pos.y - 0.45f, nz + m)
     || player_clipbox(p->pos.x - 0.45f, p->pos.y + 0.45f, nz + m)
     || player_clipbox(p->pos.x + 0.45f, p->pos.y - 0.45f, nz + m)
     || player_clipbox(p->pos.x + 0.45f, p->pos.y + 0.45f, nz + m)) {
        if (p->physics.velocity.z >= 0) {
            p->physics.wade = p->pos.z > 61;
            p->physics.airborne = 0;
        }
        p->physics.velocity.z = 0;
    } else
        p->pos.z = nz - offset;

    player_reposition(p);
}

int player_move(Player * p, float fsynctics, int id) {
    if (!p->alive) {
        p->physics.velocity.y -= fsynctics;
        AABB dead_bb = {.min = {0, 0, 0}, .max = {0, 0, 0}};
        aabb_set_size(&dead_bb, 0.7F, 0.15F, 0.7F);
        aabb_set_center(&dead_bb, p->pos.x + p->physics.velocity.x * fsynctics * 32.0F,
                        p->pos.y + p->physics.velocity.y * fsynctics * 32.0F,
                        p->pos.z + p->physics.velocity.z * fsynctics * 32.0F);

        if (!aabb_intersection_terrain(&dead_bb, 0)) {
            p->pos.x += p->physics.velocity.x * fsynctics * 32.0F;
            p->pos.y += p->physics.velocity.y * fsynctics * 32.0F;
            p->pos.z += p->physics.velocity.z * fsynctics * 32.0F;
        } else {
            p->physics.velocity.x *= +0.36F;
            p->physics.velocity.y *= -0.36F;
            p->physics.velocity.z *= +0.36F;
        }

        return 0;
    }

    int local = (id == local_player.id && camera.mode == CAMERAMODE_FPS);

    player_coordsystem_adjust1(p);
    float f, f2;

    // move player and perform simple physics (gravity, momentum, friction)
    if (p->physics.jump) {
        sound_create(
            local ? SOUND_LOCAL : SOUND_WORLD, sound(p->physics.wade ? SOUND_JUMP_WATER : SOUND_JUMP),
            p->pos.x, 63.0F - p->pos.z, p->pos.y
        );

        p->physics.jump       = 0;
        p->physics.velocity.z = -0.36f;
    }

    f = fsynctics; // player acceleration scalar
    if (p->physics.airborne)
        f *= 0.1f;
    else if (HASBIT(p->input.keys, INPUT_CROUCH))
        f *= 0.3f;
    else if (ISSCOPING(p) || HASBIT(p->input.keys, INPUT_SNEAK))
        f *= 0.5f;
    else if (HASBIT(p->input.keys, INPUT_SPRINT))
        f *= 1.3f;

    if ((HASBIT(p->input.keys, INPUT_UP)   || HASBIT(p->input.keys, INPUT_DOWN)) &&
        (HASBIT(p->input.keys, INPUT_LEFT) || HASBIT(p->input.keys, INPUT_RIGHT)))
        f *= SQRT; // if strafe + forward/backwards then limit diagonal velocity

    float len = hypot2f(p->orientation.x, p->orientation.y);
    float sx  = p->orientation.x / len;
    float sy  = p->orientation.y / len;

    // Servers (e.g. piqueserver) expect that player cannot move forwards/backwards while looking up.
    // https://github.com/piqueserver/piqueserver/blob/17f43a559abd6472263382aef271f93a6cb01b7e/pyspades/world_c.cpp#L690-L699
    if (HASBIT(p->input.keys, INPUT_UP)) {
        p->physics.velocity.x += p->orientation.x * f;
        p->physics.velocity.y += p->orientation.y * f;
    } else if (HASBIT(p->input.keys, INPUT_DOWN)) {
        p->physics.velocity.x -= p->orientation.x * f;
        p->physics.velocity.y -= p->orientation.y * f;
    }

    if (HASBIT(p->input.keys, INPUT_LEFT)) {
        p->physics.velocity.x += sy * f;
        p->physics.velocity.y -= sx * f;
    } else if (HASBIT(p->input.keys, INPUT_RIGHT)) {
        p->physics.velocity.x -= sy * f;
        p->physics.velocity.y += sx * f;
    }

    f = fsynctics + 1;
    p->physics.velocity.z += fsynctics;
    p->physics.velocity.z /= f; // air friction
    if (p->physics.wade)
        f = fsynctics * 6.0F + 1; // water friction
    else if (!p->physics.airborne)
        f = fsynctics * 4.0F + 1; // ground friction

    p->physics.velocity.x /= f;
    p->physics.velocity.y /= f;
    f2 = p->physics.velocity.z;
    player_boxclipmove(p, fsynctics);
    // hit ground... check if hurt

    int ret = 0;

    if (!p->physics.velocity.z && (f2 > FALL_SLOW_DOWN)) {
        // slow down on landing
        p->physics.velocity.x *= 0.5F;
        p->physics.velocity.y *= 0.5F;

        // return fall damage
        if (f2 > FALL_DAMAGE_VELOCITY) {
            f2 -= FALL_DAMAGE_VELOCITY;
            ret = f2 * f2 * FALL_DAMAGE_SCALAR;
            sound_create(local ? SOUND_LOCAL : SOUND_WORLD, sound(SOUND_HURT_FALL), p->pos.x, 63.0F - p->pos.z, p->pos.y);
        } else {
            sound_create(local ? SOUND_LOCAL : SOUND_WORLD, sound(p->physics.wade ? SOUND_LAND_WATER : SOUND_LAND), p->pos.x,
                         63.0F - p->pos.z, p->pos.y);
            ret = -1;
        }
    }

    player_coordsystem_adjust2(p);

    if (ISMOVING(p)) {
        if (window_time() - p->sound.feet_started > (HASBIT(p->input.keys, INPUT_SPRINT) ? (0.5F / 1.3F) : 0.5F)
           && (!HASBIT(p->input.keys, INPUT_CROUCH) && !HASBIT(p->input.keys, INPUT_SNEAK))
           && !p->physics.airborne
           && norm2f(p->physics.velocity.x, p->physics.velocity.z, 0.0F, 0.0F) > sqrf(0.125F)) {

            static enum WAV footstep[] = {SOUND_FOOTSTEP1, SOUND_FOOTSTEP2, SOUND_FOOTSTEP3, SOUND_FOOTSTEP4};
            static enum WAV wade[]     = {SOUND_WADE1,     SOUND_WADE2,     SOUND_WADE3,     SOUND_WADE4};

            size_t num = rand() % 4; WAV * wav = sound(p->physics.wade ? wade[num] : footstep[num]);

            if (local) sound_create(SOUND_LOCAL, wav, p->pos.x, p->pos.y, p->pos.z);
            else sound_create_sticky(wav, p, id);

            p->sound.feet_started = window_time();
        }

        if (window_time() - p->sound.feet_started_cycle > (HASBIT(p->input.keys, INPUT_SPRINT) ? (0.5F / 1.3F) : 0.5F)) {
            p->sound.feet_started_cycle = window_time();
            p->sound.feet_cylce         = !p->sound.feet_cylce;
        }
    }

    return ret;
}

int player_uncrouch(Player * p) {
    player_coordsystem_adjust1(p);

    float x1 = p->pos.x + 0.45F, y1 = p->pos.y + 0.45F, z1 = p->pos.z + 2.25F;
    float x2 = p->pos.x - 0.45F, y2 = p->pos.y - 0.45F, z2 = p->pos.z - 1.35F;

    // first check if player can lower feet (in midair)
    if (p->physics.airborne
       && !(player_clipbox(x1, y1, z1) ||
            player_clipbox(x1, y2, z1) ||
            player_clipbox(x2, y1, z1) ||
            player_clipbox(x2, y2, z1))) {
        player_coordsystem_adjust2(p);
        return 1;
    // then check if they can raise their head
    } else if (!(player_clipbox(x1, y1, z2) ||
                 player_clipbox(x1, y2, z2) ||
                 player_clipbox(x2, y1, z2) ||
                 player_clipbox(x2, y2, z2))) {
        p->pos.z         -= 0.9F;
        p->physics.eye.z -= 0.9F;
        if (&players[local_player.id] == p) last_cy += 0.9F;

        player_coordsystem_adjust2(p);
        return 1;
    }

    player_coordsystem_adjust2(p);
    return 0;
}
