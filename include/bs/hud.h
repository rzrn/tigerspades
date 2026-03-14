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

#ifndef HUD_H
#define HUD_H

#include <stdbool.h>
#include <microui.h>

#include <bs/texture.h>
#include <bs/window.h>
#include <bs/common.h>

typedef struct {
    void (*init)(void);
    void (*render_3D)(void);
    void (*render_2D)(mu_Context * ctx, float);
    void (*input_keyboard)(int key, int action, int mods, int internal);
    void (*input_mouselocation)(double x, double y);
    void (*input_mouseclick)(double x, double y, int button, int action, int mods);
    void (*input_mousescroll)(double yoffset);
    void (*input_touch)(void * finger, int action, float x, float y, float dx, float dy);
    void (*focus)(bool);
    void (*hover)(bool);
    Texture * (*ui_images)(int icon_id, bool * resize);
    bool render_world;
    bool render_localplayer;
    mu_Context * ctx;
} HUD;

extern HUD hud_ingame;
extern HUD hud_mapload;
extern HUD hud_serverlist;
extern HUD hud_settings;
extern HUD hud_controls;

extern HUD * hud_active;

extern const char * hud_serverlist_popup;

extern Text * hud_game_chat_selected;

#define HUD_FLAG_INDEX_START 64

void hud_show_popup(const char * format, ...);

void hud_change(HUD *);
void hud_init(void);
void hud_mousemode(int mode);

void load_map(const char *);

typedef struct {
    RGB3f color;
    char label[64];
} LegendEntry;

typedef struct _Graph Graph;

struct _Graph {
    uint8_t index;
    size_t nrows, ncols;
    LegendEntry * legend;
    float2 * data;
    Graph * next;
};

extern Graph * hud_graph;

#endif
