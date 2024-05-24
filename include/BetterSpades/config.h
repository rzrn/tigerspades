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

#include <BetterSpades/common.h>
#include <BetterSpades/list.h>

#ifdef USE_GLFW
    #include <BetterSpades/GUI/glfw.h>
#endif

#ifdef USE_SDL
    #include <BetterSpades/GUI/sdl.h>
#endif

#ifdef USE_GLUT
    #include <BetterSpades/GUI/glut.h>
#endif

#ifdef USE_COCOA
    #include <BetterSpades/GUI/Cocoa.h>
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
    int   opengl14;
    int   color_correction;
    int   shadow_entities;
    int   ambient_occlusion;
    float render_distance;
    int   window_width;
    int   window_height;
    int   multisamples;
    int   player_arms;
    int   fullscreen;
    int   greedy_meshing;
    int   vsync;
    float mouse_sensitivity;
    int   show_news;
    int   volume;
    int   voxlap_models;
    int   force_displaylist;
    int   invert_y;
    int   smooth_fog;
    float camera_fov;
    int   hold_down_sights;
    int   chat_shadow;
    int   scale;
    int   tracing_enabled;
    int   trajectory_length;
    int   projectile_count;
    int   show_minimap;
    int   toggle_crouch;
    int   toggle_sprint;
    int   enable_shadows;
    int   enable_particles;
    int   smooth_orientation;
} Options;

extern Options settings, settings_tmp;

extern List config_keys;

typedef struct {
    int  internal;
    int  def;
    int  original;
    int  toggle;
    char name[24];
    char display[24];
    char category[24];
} ConfigKeyPair;

typedef struct {
    int  key;
    char value[128];
} Keybind;

enum {
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_FLOAT,
    CONFIG_TYPE_INT
};

typedef struct {
    void * value;
    int    type;
    int    min;
    int    max;
    char   name[32];
    char   help[32];
    char   category[32];
    int    defaults[8];
    int    defaults_length;
    void (*label_callback)(char * buffer, size_t length, int value, size_t index);
} Setting;

extern char * config_filepath;

extern List config_settings;

ConfigKeyPair * config_key(int key);

int config_key_translate(int key, int dir, int * results);
void config_key_reset_togglestates();
void config_reload(void);
void config_save(void);

extern List config_keybind;

#endif
