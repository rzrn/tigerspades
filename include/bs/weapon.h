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

#ifndef WEAPON_H
#define WEAPON_H

#include <bs/player.h>
#include <bs/sound.h>
#include <bs/model.h>

typedef enum {
    RIFLE_SAFE,
    RIFLE_SEMI,
    SMG_SAFE,
    SMG_SEMI,
    SMG_BURST,
    SMG_AUTO,
    SHOTGUN_SAFE,
    SHOTGUN_PUMP
} WeaponFireMode;

float weapon_recoil_anim(Weapon);
int weapon_block_damage(Weapon);
float weapon_delay(Weapon);

WAV * weapon_sound(Weapon);
WAV * weapon_sound_reload(Weapon);

int weapon_ammo(Weapon);
int weapon_ammo_reserved(Weapon);

kv6 * weapon_casing(Weapon);
kv6 * weapon_model(Weapon);

void weapon_update(void);
void weapon_set(bool restock);
void weapon_reload(void);
bool weapon_reloading(void);
int weapon_can_reload(void);
void weapon_reload_abort(void);
void weapon_shoot(void);

Vector3f weapon_spread(Player *, const Vector3f);
Euler2d weapon_recoil(Weapon);

extern float weapon_reload_start, weapon_last_shot;

WeaponFireMode weapon_firemode_default(Weapon);
WeaponFireMode weapon_firemode_cycle(WeaponFireMode);
int weapon_firemode_burst(WeaponFireMode);
const char * weapon_firemode_label(WeaponFireMode);

extern WeaponFireMode weapon_firemode;
extern int weapon_burst;

#endif
