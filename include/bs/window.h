/*
    Copyright © 2017–2022 ByteBit
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

#ifndef WINDOW_H
#define WINDOW_H

#include <stddef.h>

#include <bs/common.h>

enum {
    WINDOW_PRESS,
    WINDOW_RELEASE,
    WINDOW_REPEAT,
};

enum {
    TOUCH_DOWN,
    TOUCH_MOVE,
    TOUCH_UP,
};

typedef enum {
    WINDOW_KEY_UP,
    WINDOW_KEY_LEFT,
    WINDOW_KEY_DOWN,
    WINDOW_KEY_RIGHT,
    WINDOW_KEY_SPACE,
    WINDOW_KEY_SPRINT,
    WINDOW_KEY_CROUCH,
    WINDOW_KEY_SNEAK,

    WINDOW_KEY_CURSOR_UP,
    WINDOW_KEY_CURSOR_DOWN,
    WINDOW_KEY_CURSOR_LEFT,
    WINDOW_KEY_CURSOR_RIGHT,
    WINDOW_KEY_PICKCOLOR,

    WINDOW_KEY_TOOL1,
    WINDOW_KEY_TOOL2,
    WINDOW_KEY_TOOL3,
    WINDOW_KEY_TOOL4,
    WINDOW_KEY_RELOAD,
    WINDOW_KEY_CHANGEWEAPON,
    WINDOW_KEY_LASTTOOL,
    WINDOW_KEY_FIREMODE,

    WINDOW_KEY_ESCAPE,
    WINDOW_KEY_VOLUME_UP,
    WINDOW_KEY_VOLUME_DOWN,
    WINDOW_KEY_CHAT,
    WINDOW_KEY_TEAM_CHAT,
    WINDOW_KEY_FULLSCREEN,
    WINDOW_KEY_SCREENSHOT,
    WINDOW_KEY_CHANGETEAM,
    WINDOW_KEY_COMMAND,
    WINDOW_KEY_HIDEHUD,
    WINDOW_KEY_SAVE_MAP,
    WINDOW_KEY_RELEASE_MOUSE,

    WINDOW_KEY_TAB,
    WINDOW_KEY_MAP,
    WINDOW_KEY_NETWORKSTATS,
    WINDOW_KEY_DEBUG,
    WINDOW_KEY_TRACE_CLEAR,
    WINDOW_KEY_CHAT_CLEAR,

    WINDOW_KEY_CYCLE_CAMERA,
    WINDOW_KEY_TOGGLE_ALIVE,
    WINDOW_KEY_RESPAWN,
    WINDOW_KEY_RESTOCK,
    WINDOW_KEY_TEAM_COLOR,

    WINDOW_KEY_SHIFT,
    WINDOW_KEY_BACKSPACE,
    WINDOW_KEY_ENTER,
    WINDOW_KEY_V,
    WINDOW_KEY_SELECT1,
    WINDOW_KEY_SELECT2,
    WINDOW_KEY_SELECT3,
    WINDOW_KEY_SELECT4,
    WINDOW_KEY_UNKNOWN,

    WINDOW_KEY_FIRST = WINDOW_KEY_UP,
    WINDOW_KEY_LAST  = WINDOW_KEY_UNKNOWN
} WindowKey;

typedef enum {
    WINDOW_MOUSE_LMB,
    WINDOW_MOUSE_MMB,
    WINDOW_MOUSE_RMB,
} WindowButton;

enum {
    WINDOW_CURSOR_DISABLED,
    WINDOW_CURSOR_ENABLED,
};

typedef struct {
    int64_t finger; float down_time; int full;
    struct { float x, y; } start;
} WindowFinger;

extern int window_pressed_keys[];

float window_time(void);

enum {
    SCREEN_NONE        = 0,
    SCREEN_TEAM_SELECT = 1,
    SCREEN_GUN_SELECT  = 2,
};

extern int screen_current;

int window_key_down(int key);
void window_key_reset_togglestates(void);

void window_textinput(int allow);
void window_keyname(int keycode, char * output, size_t length);
const char * window_clipboard(void);
void window_mousemode(int mode);
void window_mouseloc(double * x, double * y);
void window_setmouseloc(double x, double y);
void window_swapping(int value);
void window_videomode(bool fullscreen);
void window_fromsettings(void);
void window_deinit(void);
int window_cpucores(void);

float window_aspect(void);

void window_settitle(char *);
void window_title(const char * suffix);
void window_sendkey(int action, int keycode, int mod);

int window_get_mousemode(void);

void window_init(const char * title, int *, char **);

#endif
