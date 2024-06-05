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

#include <BetterSpades/aabb.h>
#include <BetterSpades/network.h>

#define PLAYERS_MAX 256 // just because 32 players are not enough

#if PLAYERS_MAX < 256
    #define IDVALID(id) true
#else
    #define IDVALID(id) ((id) < PLAYERS_MAX)
#endif

typedef enum {
    GAMEMODE_CTF = 0,
    GAMEMODE_TC  = 1
} GameMode;

enum {
    TEAM_1          = 0,
    TEAM_2          = 1,
    TEAM_SPECTATOR  = 255
};

#define TEAM(t) (((t) == TEAM_1 || (t) == TEAM_2) ? (t) : TEAM_SPECTATOR)

typedef struct {
    char name[11];
    RGB3i color;
} Team;

typedef union _IntelLocation {
    uint8_t held;
    Vector3f dropped;
} IntelLocation;

typedef struct {
    Vector3f pos;
    uint8_t team;
} Territory;

typedef union _Gamemode {
    struct {
        uint8_t team_1_score, team_2_score;
        uint8_t capture_limit, intels;
        IntelLocation team_1_intel_location, team_2_intel_location;
        Vector3f team_1_base, team_2_base;
    } ctf;

    struct {
        unsigned char territory_count;
        Territory territory[16];
    } tc;
} Gamemode;

typedef struct {
    Team team_1, team_2;
    unsigned char gamemode_type;
    Gamemode gamemode;
    struct {
        unsigned char team_capturing, tent;
        float progress, rate, update;
    } progressbar;
} GameState;

extern GameState gamestate;

typedef struct { bool lmb, mmb, rmb; } MouseButtons;
extern MouseButtons button_map;

typedef struct {
    uint8_t id, health, blocks, grenades, ammo, ammo_reserved;
    uint8_t last_tool, respawn_time, respawn_cnt_last;
    float death_time, last_damage_timer; Vector3f last_damage;
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

bool player_intersection_exists(Hit * s);
int player_intersection_choose(Hit * s, float * distance);

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
    unsigned char team, weapon, held_item;
    unsigned char alive, connected;
    float item_showup, item_disabled, items_show_start;
    unsigned char items_show;
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

void player_on_held_item_change(Player *);
int player_can_spectate(Player *);
float player_section_height(int section);
void player_init(void);
float player_height(const Player *);
float player_height2(const Player *);
void player_reposition(Player *);
void player_update(float dt, int locked);
void player_render_all(void);
void player_render(Player * p, int id);
void player_collision(const Player *, Ray * ray, Hit * intersects);
void player_reset(Player *);
int player_move(Player *, float fsynctics, int id);
int player_uncrouch(Player *);

#define ISSCOPING(player) (HASBIT((player)->input.buttons, BUTTON_SECONDARY) && (player)->held_item == TOOL_GUN)

#define ISMOVING(player) (HASBIT((player)->input.keys, INPUT_UP)   || \
                          HASBIT((player)->input.keys, INPUT_DOWN) || \
                          HASBIT((player)->input.keys, INPUT_LEFT) || \
                          HASBIT((player)->input.keys, INPUT_RIGHT))

#endif
