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

void weapon_update(void);
void weapon_set(bool restock);
void weapon_reload(void);
bool weapon_reloading(void);
int weapon_can_reload(void);
void weapon_reload_abort(void);
void weapon_shoot(void);
int weapon_block_damage(int gun);
float weapon_delay(int gun);
int weapon_ammo(int gun);
int weapon_ammo_reserved(int gun);
kv6 * weapon_casing(int gun);
kv6 * weapon_model(int gun);
float weapon_recoil_anim(int gun);
WAV * weapon_sound(int gun);
WAV * weapon_sound_reload(int gun);
void weapon_spread(Player * p, float * d);
void weapon_recoil(int gun, double * horiz_recoil, double * vert_recoil);

extern float weapon_reload_start, weapon_last_shot;

#endif
