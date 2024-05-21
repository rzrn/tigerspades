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

#ifndef HUD_H
#define HUD_H

#include <stdbool.h>
#include <microui.h>

#include <BetterSpades/texture.h>
#include <BetterSpades/window.h>
#include <BetterSpades/common.h>

typedef struct {
    void (*init)();
    void (*render_3D)();
    void (*render_2D)(mu_Context * ctx, float);
    void (*input_keyboard)(int key, int action, int mods, int internal);
    void (*input_mouselocation)(double x, double y);
    void (*input_mouseclick)(double x, double y, int button, int action, int mods);
    void (*input_mousescroll)(double yoffset);
    void (*input_touch)(void * finger, int action, float x, float y, float dx, float dy);
    void (*focus)(bool);
    void (*hover)(bool);
    Texture * (*ui_images)(int icon_id, bool * resize);
    char render_world;
    char render_localplayer;
    mu_Context * ctx;
} HUD;

typedef struct {
    int     current, max;
    char    name[32];
    char    map[21];
    char    gamemode[8];
    int     ping;
    char    identifier[32];
    char    country[4];
    Version version;
} Server;

extern HUD hud_ingame;
extern HUD hud_mapload;
extern HUD hud_serverlist;
extern HUD hud_settings;
extern HUD hud_controls;

extern HUD * hud_active;
extern WindowInstance * hud_window;

extern bool offline;
extern char serverlist_url[2048], newslist_url[2048];

#define HUD_FLAG_INDEX_START 64

void hud_change(HUD *);
void hud_init();
void hud_mousemode(int mode);

void load_map(const char *);

#endif
