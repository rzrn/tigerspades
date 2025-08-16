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

#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

#include <bs/aabb.h>
#include <bs/network.h>

typedef struct {
    char name[11];
    RGB3i color;
} TeamObject;

typedef struct {
    unsigned int team1_score, team2_score, capture_limit;
    bool team1_has_intel, team2_has_intel;
    uint8_t team1_carrier, team2_carrier;
    Vector3f team1_flag, team2_flag;
    Vector3f team1_base, team2_base;
} CTFState;

typedef struct {
    Vector3f pos;
    uint8_t team;
} Territory;

typedef struct {
    unsigned char team_capturing, tent;
    float progress, rate, last_update;

    unsigned char territory_count;
    Territory territory[16];
} TCState;

typedef struct {
    GameMode mode;
    TeamObject team1, team2;
    CTFState ctf; TCState tc;
} GameState;

extern GameState gamestate;

typedef struct { bool lmb, mmb, rmb; } MouseButtons;
extern MouseButtons button_map;

typedef struct {
    uint8_t id, health, blocks, grenades, ammo, ammo_reserved;
    uint8_t last_tool, respawn_time, respawn_cnt_last;
    float death_time, last_damage_timer, last_kill_timer; Vector3f last_damage;
    bool drag_active; Vector3i drag; int color[2];
} LocalPlayer;

extern LocalPlayer local_player;

extern int default_team, default_gun;

extern int player_intersection_type;
extern int player_intersection_player;
extern float player_intersection_dist;

typedef struct {
    bool head;
    bool torso;
    bool leg_left;
    bool leg_right;
    bool arms;

    struct {
        float head;
        float torso;
        float leg_left;
        float leg_right;
        float arms;
    } distance;
} Hit;

bool player_intersection_exists(Hit *);
HitType player_intersection_choose(Hit *, float * distance);

typedef struct {
    char name[17];
    Vector3f pos, orientation;
    AABB bb_2d;
    Vector3f orientation_smooth;
    Vector3f gun_pos, casing_dir;
    float gun_shoot_timer;
    int ammo, ammo_reserved;
    float spade_use_timer;
    unsigned char spade_used, spade_use_type;
    unsigned int score;
    Team team; Weapon weapon; Tool tool;
    unsigned char alive, connected;
    float item_showup, item_disabled, items_show_start;
    bool items_show;
    RGB3i block;

    struct {
        unsigned char keys, buttons;
    } input;

    struct {
        float lmb, rmb;
    } start;

    struct {
        unsigned char jump, airborne, wade;
        float lastclimb;
        Vector3f velocity, eye;
    } physics;

    struct {
        float feet_started, feet_started_cycle;
        char feet_cylce;
        float tool_started;
    } sound;
} Player;

extern Player players[PLAYERS_MAX];
// pyspades/pysnip/piqueserver sometimes uses ids that are out of range

float player_section_height(HitType);

void player_on_tool_change(void);
bool player_can_spectate(Player *);
void player_init(void);
float player_height(const Player *);
float player_height2(const Player *);
void player_reposition(Player *);
void player_update_position(float);
void player_update_orientation(float);
void player_render_all(void);
void player_render(Player * p, int id);
void player_collision(const Player *, Ray *, Hit *);
void player_reset(Player *);
int player_move(Player *, float fsynctics, int id);
int player_uncrouch(Player *);

#define ISFIRING(player) (HASBIT((player)->input.buttons, BUTTON_PRIMARY))

#define ISSCOPING(player) (HASBIT((player)->input.buttons, BUTTON_SECONDARY) && (player)->tool == TOOL_WEAPON)

#define ISMOVING(player) (HASBIT((player)->input.keys, INPUT_UP)   || \
                          HASBIT((player)->input.keys, INPUT_DOWN) || \
                          HASBIT((player)->input.keys, INPUT_LEFT) || \
                          HASBIT((player)->input.keys, INPUT_RIGHT))

#endif
