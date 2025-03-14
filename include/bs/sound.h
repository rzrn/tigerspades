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

#ifndef SOUND_H
#define SOUND_H

#include <bs/player.h>

#define SOUND_SCALE 0.6F

typedef enum {
    SOUND_WORLD,
    SOUND_LOCAL,
} SoundSpace;

extern bool sound_enabled;

typedef struct _WAV WAV;

enum WAV {
    SOUND_CHAT,
    SOUND_FOOTSTEP1, SOUND_FOOTSTEP2, SOUND_FOOTSTEP3, SOUND_FOOTSTEP4,
    SOUND_WADE1, SOUND_WADE2, SOUND_WADE3, SOUND_WADE4,
    SOUND_JUMP, SOUND_JUMP_WATER, SOUND_LAND, SOUND_LAND_WATER, SOUND_HURT_FALL,
    SOUND_EXPLODE, SOUND_EXPLODE_WATER, SOUND_GRENADE_BOUNCE, SOUND_GRENADE_PIN,
    SOUND_RIFLE_SHOOT, SOUND_RIFLE_RELOAD, SOUND_SMG_SHOOT, SOUND_SMG_RELOAD,
    SOUND_SHOTGUN_SHOOT, SOUND_SHOTGUN_RELOAD, SOUND_SHOTGUN_COCK,
    SOUND_HITGROUND, SOUND_HITPLAYER, SOUND_BUILD,
    SOUND_SPADE_WOOSH, SOUND_SPADE_WHACK, SOUND_DEATH,
    SOUND_BEEP1, SOUND_BEEP2, SOUND_SWITCH, SOUND_EMPTY, SOUND_INTRO,
    SOUND_DEBRIS, SOUND_BOUNCE, SOUND_IMPACT, SOUND_PICKUP, SOUND_HORN,

    SOUND_FIRST = SOUND_CHAT,
    SOUND_LAST  = SOUND_HORN
};

extern WAV * sound(enum WAV);

void sound_volume(float vol);
void sound_create_sticky(WAV *, Player * player, int player_id);
void sound_create(SoundSpace, WAV *, float x, float y, float z);
void sound_update(void);
void sound_load(WAV *, const char * filename, float min, float max);
void sound_init(void);
void sound_deinit(void);

#endif
