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

#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

#include <bs/common.h>
#include <bs/list.h>

#ifdef USE_GLFW
    #include <bs/gui/glfw.h>
#endif

#ifdef USE_SDL
    #include <bs/gui/sdl.h>
#endif

#ifdef USE_GLUT
    #include <bs/gui/glut.h>
#endif

#ifdef USE_COCOA
    #include <bs/gui/cocoa.h>
#endif

typedef struct {
    char section[32];
    char name[32];
    char value[32];
} ConfigFileEntry;

typedef struct {
    char  name[16];
    int   min_lan_port;
    int   max_lan_port;
    bool  opengl14;
    bool  ambient_occlusion;
    float render_distance;
    int   window_width;
    int   window_height;
    int   multisamples;
    bool  windowed;
    bool  greedy_meshing;
    int   vsync;
    float mouse_sensitivity;
    bool  show_news;
    int   volume;
    bool  voxlap_models;
    bool  force_displaylist;
    bool  invert_y;
    bool  smooth_fog;
    float camera_fov;
    bool  hold_down_sights;
    bool  chat_shadow;
    int   scale;
    bool  tracing_enabled;
    int   trajectory_length;
    int   projectile_count;
    bool  show_minimap;
    bool  fixed_minimap;
    bool  toggle_crouch;
    bool  toggle_sprint;
    bool  enable_shadows;
    bool  enable_particles;
    bool  smooth_orientation;
    bool  map_cache;
    bool  chat_beep;
    bool  connect_beep;
    bool  disconnect_beep;
    bool  team_change_beep;
    bool  show_crosshair;
    bool  show_health;
    bool  show_ammo;
    bool  show_hotbar;
    bool  show_friendly_tag;
    bool  report_client_version;
    bool  left_handed;
    bool  kill_indicator;
    bool  persistent_block_color;
    bool  show_iron_sight;
    float deadzone_horiz;
    float deadzone_vert;
    bool  free_crosshair;
    bool  render_player;
} Options;

extern Options settings, settings_tmp;

static inline bool deadzone_enabled(void)
{ return settings.deadzone_horiz > 0 || settings.deadzone_vert > 0; }

typedef struct {
    int          keycode;
    int          original;
    bool         toggle;
    const char * name;
    const char * display;
    const char * category;
} ConfigKey;

typedef struct {
    int  key;
    char value[128];
} Keybind;

enum {
    CONFIG_TYPE_BOOLEAN,
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_FLOAT,
    CONFIG_TYPE_INT
};

typedef void Label(char *, size_t, void *);

typedef struct {
    void *       value;
    int          type;
    int          size;
    int          mini, maxi;
    float        minf, maxf;
    const char * name;
    const char * display;
    const char * help;
    const char * category;
    int          defaults[8];
    int          defaults_length;
    Label *      label;
} Setting;

extern char * config_filepath;

extern Setting * config_settings_begin;
extern Setting * config_settings_end;

ConfigKey * config_key(int key);

void config_init(void);
void config_save(void);

extern List config_keybind;

#endif
