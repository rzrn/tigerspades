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

#include <stdlib.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <bs/window.h>
#include <bs/file.h>
#include <bs/config.h>
#include <bs/sound.h>
#include <bs/model.h>
#include <bs/camera.h>
#include <bs/utils.h>

#include <bs/gui/toolkit.h>

#include <ini.h>

static void config_label_scale(char * buffer, size_t length, int value, size_t index) {
    if (value == 0)
        snprintf(buffer, length, "Auto");
    else
        snprintf(buffer, length, "%i", value);
}

static void config_label_pixels(char * buffer, size_t length, int value, size_t index) {
    if (value == 800 || value == 600)
        snprintf(buffer, length, "default: %i px", value);
    else
        snprintf(buffer, length, "%i px", value);
}

static void config_label_vsync(char * buffer, size_t length, int value, size_t index) {
    if (value == 0)
        snprintf(buffer, length, "disabled");
    else if (value == 1)
        snprintf(buffer, length, "enabled");
    else
        snprintf(buffer, length, "max %i fps", value);
}

static void config_label_msaa(char * buffer, size_t length, int value, size_t index) {
    if (index == 0)
        snprintf(buffer, length, "No MSAA");
    else
        snprintf(buffer, length, "%ix MSAA", value);
}

static void config_label_left_handed(char * buffer, size_t length, int value, size_t index) {
    snprintf(buffer, length, index == 0 ? "Right" : "Left");
}

ConfigKey _config_key[] = {
    [WINDOW_KEY_UP]            = {.keycode = TOOLKIT_KEY_W,            .name = "move_forward",      .display = "Forward", .category = "Movement"},
    [WINDOW_KEY_LEFT]          = {.keycode = TOOLKIT_KEY_A,            .name = "move_left",         .display = "Left"},
    [WINDOW_KEY_DOWN]          = {.keycode = TOOLKIT_KEY_S,            .name = "move_backward",     .display = "Backward"},
    [WINDOW_KEY_RIGHT]         = {.keycode = TOOLKIT_KEY_D,            .name = "move_right",        .display = "Right"},
    [WINDOW_KEY_SPACE]         = {.keycode = TOOLKIT_KEY_SPACE,        .name = "jump",              .display = "Jump"},
    [WINDOW_KEY_SPRINT]        = {.keycode = TOOLKIT_KEY_SHIFT,        .name = "sprint",            .display = "Sprint"},
    [WINDOW_KEY_CROUCH]        = {.keycode = TOOLKIT_KEY_CONTROL,      .name = "crouch",            .display = "Crouch"},
    [WINDOW_KEY_SNEAK]         = {.keycode = TOOLKIT_KEY_V,            .name = "sneak",             .display = "Sneak"},

    [WINDOW_KEY_CURSOR_UP]     = {.keycode = TOOLKIT_KEY_CURSOR_UP,    .name = "cube_color_up",     .display = "Color up", .category = "Block"},
    [WINDOW_KEY_CURSOR_DOWN]   = {.keycode = TOOLKIT_KEY_CURSOR_DOWN,  .name = "cube_color_down",   .display = "Color down"},
    [WINDOW_KEY_CURSOR_LEFT]   = {.keycode = TOOLKIT_KEY_CURSOR_LEFT,  .name = "cube_color_left",   .display = "Color left"},
    [WINDOW_KEY_CURSOR_RIGHT]  = {.keycode = TOOLKIT_KEY_CURSOR_RIGHT, .name = "cube_color_right",  .display = "Color right"},
    [WINDOW_KEY_PICKCOLOR]     = {.keycode = TOOLKIT_KEY_E,            .name = "cube_color_sample", .display = "Pick color"},

    [WINDOW_KEY_TOOL1]         = {.keycode = TOOLKIT_KEY_1,            .name = "tool_spade",        .display = "Select spade", .category = "Tools & Weapons"},
    [WINDOW_KEY_TOOL2]         = {.keycode = TOOLKIT_KEY_2,            .name = "tool_block",        .display = "Select block"},
    [WINDOW_KEY_TOOL3]         = {.keycode = TOOLKIT_KEY_3,            .name = "tool_gun",          .display = "Select gun"},
    [WINDOW_KEY_TOOL4]         = {.keycode = TOOLKIT_KEY_4,            .name = "tool_grenade",      .display = "Select grenade"},
    [WINDOW_KEY_RELOAD]        = {.keycode = TOOLKIT_KEY_R,            .name = "reload",            .display = "Reload"},
    [WINDOW_KEY_CHANGEWEAPON]  = {.keycode = TOOLKIT_KEY_PERIOD,       .name = "change_weapon",     .display = "Gun select"},
    [WINDOW_KEY_LASTTOOL]      = {.keycode = TOOLKIT_KEY_Q,            .name = "last_tool",         .display = "Last tool"},

    [WINDOW_KEY_ESCAPE]        = {.keycode = TOOLKIT_KEY_ESCAPE,       .name = "quit_game",         .display = "Quit", .category = "Game"},
    [WINDOW_KEY_VOLUME_UP]     = {.keycode = TOOLKIT_KEY_ADD,          .name = "volume_up",         .display = "Volume up"},
    [WINDOW_KEY_VOLUME_DOWN]   = {.keycode = TOOLKIT_KEY_SUBTRACT,     .name = "volume_down",       .display = "Volume down"},
    [WINDOW_KEY_CHAT]          = {.keycode = TOOLKIT_KEY_T,            .name = "chat_global",       .display = "Chat"},
    [WINDOW_KEY_TEAM_CHAT]     = {.keycode = TOOLKIT_KEY_Y,            .name = "chat_team",         .display = "Team chat"},
    [WINDOW_KEY_FULLSCREEN]    = {.keycode = TOOLKIT_KEY_F11,          .name = "fullscreen",        .display = "Fullscreen"},
    [WINDOW_KEY_SCREENSHOT]    = {.keycode = TOOLKIT_KEY_F5,           .name = "screenshot",        .display = "Screenshot"},
    [WINDOW_KEY_CHANGETEAM]    = {.keycode = TOOLKIT_KEY_COMMA,        .name = "change_team",       .display = "Team select"},
    [WINDOW_KEY_COMMAND]       = {.keycode = TOOLKIT_KEY_SLASH,        .name = "chat_command",      .display = "Command"},
    [WINDOW_KEY_HIDEHUD]       = {.keycode = TOOLKIT_KEY_F6,           .name = "hide_hud",          .display = "Hide HUD", .toggle = true},
    [WINDOW_KEY_SAVE_MAP]      = {.keycode = TOOLKIT_KEY_F9,           .name = "save_map",          .display = "Save map"},
    [WINDOW_KEY_RELEASE_MOUSE] = {.keycode = TOOLKIT_KEY_BACKSLASH,    .name = "release_mouse",     .display = "Release mouse"},

    [WINDOW_KEY_TAB]           = {.keycode = TOOLKIT_KEY_TAB,          .name = "view_score",        .display = "Score", .category = "Information"},
    [WINDOW_KEY_MAP]           = {.keycode = TOOLKIT_KEY_M,            .name = "view_map",          .display = "Map", .toggle = true},
    [WINDOW_KEY_NETWORKSTATS]  = {.keycode = TOOLKIT_KEY_F12,          .name = "network_stats",     .display = "Network stats", .toggle = true},
    [WINDOW_KEY_DEBUG]         = {.keycode = TOOLKIT_KEY_F3,           .name = "debug",             .display = "Debug screen", .toggle = true},
    [WINDOW_KEY_TRACE_CLEAN]   = {.keycode = TOOLKIT_KEY_F4,           .name = "trace_clean",       .display = "Clean up bullets"},

    [WINDOW_KEY_CYCLE_CAMERA]  = {.keycode = TOOLKIT_KEY_F2,           .name = "cycle_camera",      .display = "Change camera mode", .category = "Local game"},
    [WINDOW_KEY_TOGGLE_ALIVE]  = {.keycode = TOOLKIT_KEY_F4,           .name = "toggle_alive",      .display = "Toggle aliveness"},
    [WINDOW_KEY_RESPAWN]       = {.keycode = TOOLKIT_KEY_F7,           .name = "respawn",           .display = "Respawn"},
    [WINDOW_KEY_RESTOCK]       = {.keycode = TOOLKIT_KEY_F8,           .name = "restock",           .display = "Restock"},
    [WINDOW_KEY_TEAM_COLOR]    = {.keycode = TOOLKIT_KEY_F10,          .name = "team_color",        .display = "Change team color"},

    [WINDOW_KEY_SHIFT]         = {.keycode = TOOLKIT_KEY_SHIFT,        .display = NULL},
    [WINDOW_KEY_BACKSPACE]     = {.keycode = TOOLKIT_KEY_BACKSPACE,    .display = NULL},
    [WINDOW_KEY_ENTER]         = {.keycode = TOOLKIT_KEY_ENTER,        .display = NULL},
    [WINDOW_KEY_V]             = {.keycode = TOOLKIT_KEY_V,            .display = NULL},
    [WINDOW_KEY_SELECT1]       = {.keycode = TOOLKIT_KEY_1,            .display = NULL},
    [WINDOW_KEY_SELECT2]       = {.keycode = TOOLKIT_KEY_2,            .display = NULL},
    [WINDOW_KEY_SELECT3]       = {.keycode = TOOLKIT_KEY_3,            .display = NULL},
    [WINDOW_KEY_UNKNOWN]       = {.keycode = 0,                        .display = NULL},
};

Setting config_settings[] = {
    {
        .value    = settings_tmp.name,
        .type     = CONFIG_TYPE_STRING,
        .max      = sizeof(settings.name) - 1,
        .name     = "name",
        .display  = "Name",
        .help     = "Ingame player name",
        .category = "Game"
    },
    {
        .value    = &settings_tmp.show_minimap,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "show_minimap",
        .display  = "Show minimap"
    },
    {
        .value    = &settings_tmp.show_crosshair,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "show_crosshair",
        .display  = "Enable crosshair"
    },
    {
        .value    = &settings_tmp.show_health,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "show_health",
        .display  = "Show health",
    },
    {
        .value    = &settings_tmp.show_ammo,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "show_ammo",
        .display  = "Show ammo",
    },
    {
        .value    = &settings_tmp.min_lan_port,
        .type     = CONFIG_TYPE_INT,
        .max      = INT_MAX,
        .display  = "Minimum LAN port",
        .name     = "min_lan_port",
        .help     = "First port to scan for LAN games"
    },
    {
        .value    = &settings_tmp.max_lan_port,
        .type     = CONFIG_TYPE_INT,
        .max      = INT_MAX,
        .display  = "Maximum LAN port",
        .name     = "max_lan_port",
        .help     = "Last port to scan for LAN games"
    },
    {
        .value    = &settings_tmp.map_cache,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .display  = "Use map cache",
        .name     = "map_cache",
        .help     = "Can use a lot of disk space"
    },
    {
        .value    = &settings_tmp.report_client_version,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .display  = "Report client version",
        .name     = "report_client_version",
        .help     = "Server can enable extensions based on this"
    },
    {
        .value    = &settings_tmp.persistent_block_color,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .display  = "Persistent block color",
        .name     = "persistent_block_color",
        .help     = "Restore block color after respawn"
    },
    {
        .value    = &settings_tmp.left_handed,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "left_handed",
        .display  = "Main hand",
        .help     = "Affects only your local character",
        .label    = config_label_left_handed
    },
    {
        .value    = &settings_tmp.mouse_sensitivity,
        .type     = CONFIG_TYPE_FLOAT,
        .min      = 0,
        .max      = INT_MAX,
        .display  = "Mouse sensitivity",
        .name     = "mouse_sensitivity",
        .category = "Control"
    },
    {
        .value    = &settings_tmp.invert_y,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .display  = "Invert Y",
        .name     = "inverty",
        .help     = "Invert vertical mouse movement"
    },
    {
        .value    = &settings_tmp.hold_down_sights,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Only aim while pressing RMB",
        .name     = "hold_down_sights",
        .display  = "Hold down sights"
    },
    {
        .value    = &settings_tmp.toggle_crouch,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "toggle_crouch",
        .display  = "Toggle crouch"
    },
    {
        .value    = &settings_tmp.toggle_sprint,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "toggle_sprint",
        .display  = "Toggle sprint"
    },
    {
        .value    = &settings_tmp.volume,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 10,
        .name     = "vol",
        .display  = "Volume",
        .category = "Interface"
    },
    {
        .value           = &settings_tmp.scale,
        .type            = CONFIG_TYPE_INT,
        .min             = 0,
        .max             = INT_MAX,
        .name            = "scale",
        .display         = "GUI scale",
        .defaults        = {0, 1, 2, 4, 8, 16, 32, 64},
        .defaults_length = 8,
        .label           = config_label_scale
    },
    {
        .value    = &settings_tmp.chat_shadow,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "chat_shadow",
        .help     = "Dark chat background",
        .display  = "Chat shadow"
    },
    {
        .value    = &settings_tmp.show_news,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "show_news",
        .display  = "Show news",
        .help     = "Show news on server list"
    },
    {
        .value    = &settings_tmp.chat_beep,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "chat_beep",
        .display  = "Enable chat alert",
        .help     = "Beep sound on new messages"
    },
    {
        .value    = &settings_tmp.connect_beep,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "connect_beep",
        .display  = "Enable connection alert",
        .help     = "Beep sound when a player connects"
    },
    {
        .value    = &settings_tmp.disconnect_beep,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "disconnect_beep",
        .display  = "Enable disconnection alert",
        .help     = "Beep sound when a player disconnects"
    },
    {
        .value    = &settings_tmp.kill_indicator,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "kill_indicator",
        .help     = "Confirmation sound + crosshair blink",
        .display  = "Enable kill indicator"
    },
    {
        .value    = &settings_tmp.camera_fov,
        .type     = CONFIG_TYPE_FLOAT,
        .min      = CAMERA_DEFAULT_FOV,
        .max      = CAMERA_MAX_FOV,
        .name     = "camera_fov",
        .display  = "Camera FOV",
        .help     = "Field of View in degrees",
        .category = "Graphics"
    },
    {
        .value           = &settings_tmp.window_width,
        .type            = CONFIG_TYPE_INT,
        .min             = 0,
        .max             = INT_MAX,
        .name            = "xres",
        .display         = "Game width",
        .defaults        = {640, 800, 854, 1024, 1280, 1920, 3840},
        .defaults_length = 7,
        .help            = "Default: 800",
        .label           = config_label_pixels
    },
    {
        .value           = &settings_tmp.window_height,
        .type            = CONFIG_TYPE_INT,
        .min             = 0,
        .max             = INT_MAX,
        .name            = "yres",
        .display         = "Game height",
        .defaults        = {480, 600, 720, 768, 1024, 1080, 2160},
        .defaults_length = 7,
        .help            = "Default: 600",
        .label           = config_label_pixels
    },
    {
        .value           = &settings_tmp.vsync,
        .type            = CONFIG_TYPE_INT,
        .min             = 0,
        .max             = INT_MAX,
        .name            = "vsync",
        .display         = "V-Sync",
        .help            = "Limits your game's fps",
        .defaults        = {0, 1, 20, 30, 60, 120, 144, 240},
        .defaults_length = 8,
        .label           = config_label_vsync
    },
    {
        .value    = &settings_tmp.windowed,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "windowed",
        .display  = "Windowed"
    },
    {
        .value           = &settings_tmp.multisamples,
        .type            = CONFIG_TYPE_INT,
        .min             = 0,
        .max             = 16,
        .name            = "multisamples",
        .display         = "Multisamples",
        .help            = "Smooth out block edges",
        .defaults        = {0, 2, 4, 8, 16},
        .defaults_length = 5,
        .label           = config_label_msaa
    },
    {
        .value    = &settings_tmp.voxlap_models,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Render models like in voxlap",
        .name     = "voxlap_models",
        .display  = "Voxlap models"
    },
    {
        .value    = &settings_tmp.greedy_meshing,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Join similar mesh faces",
        .name     = "greedy_meshing",
        .display  = "Greedy meshing"
    },
    {
        .value    = &settings_tmp.force_displaylist,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Enable this on buggy drivers",
        .name     = "force_displaylist",
        .display  = "Force Displaylist"
    },
    {
        .value    = &settings_tmp.smooth_fog,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Enable this on buggy drivers",
        .name     = "smooth_fog",
        .display  = "Smooth fog"
    },
    {
        .value    = &settings_tmp.ambient_occlusion,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "(won't work with greedy mesh)",
        .name     = "ambient_occlusion",
        .display  = "Ambient occlusion"
    },
    {
        .value    = &settings_tmp.enable_particles,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Disable this on weak hardware",
        .name     = "enable_particles",
        .display  = "Enable particles"
    },
    {
        .value    = &settings_tmp.tracing_enabled,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Requires server support",
        .name     = "tracing_enabled",
        .display  = "Bullet tracing",
        .category = "Debug"
    },
    {
        .value    = &settings_tmp.trajectory_length,
        .type     = CONFIG_TYPE_INT,
        .min      = 16,
        .max      = 2048,
        .name     = "trajectory_length",
        .display  = "Trajectory length"
    },
    {
        .value    = &settings_tmp.projectile_count,
        .type     = CONFIG_TYPE_INT,
        .min      = 8,
        .max      = 256,
        .name     = "projectile_count",
        .display  = "Projectile count"
    },
    {
        .value    = &settings_tmp.enable_shadows,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Useful for map development",
        .name     = "enable_shadows",
        .display  = "Map shadows"
    },
    {
        .value    = &settings_tmp.smooth_orientation,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .help     = "Disable to spectate cheaters",
        .name     = "smooth_orientation",
        .display  = "Smooth orientation"
    },
    {
        .value    = &settings_tmp.player_arms,
        .type     = CONFIG_TYPE_INT,
        .min      = 0,
        .max      = 1,
        .name     = "show_player_arms",
        .display  = NULL
    }
};

Options settings = {
    .opengl14               = 1,
    .color_correction       = 0,
    .shadow_entities        = 0,
    .render_distance        = RENDER_DISTANCE,
    .name                   = "Deuce",
    .window_width           = 800,
    .window_height          = 600,
    .min_lan_port           = 32882,
    .max_lan_port           = 32892,
    .volume                 = 10,
    .invert_y               = 0,
    .windowed               = 1,
    .mouse_sensitivity      = 5.0F,
    .show_news              = 1,
    .multisamples           = 0,
    .greedy_meshing         = 0,
    .vsync                  = 1,
    .voxlap_models          = 0,
    .force_displaylist      = 0,
    .smooth_fog             = 0,
    .ambient_occlusion      = 0,
    .camera_fov             = CAMERA_DEFAULT_FOV,
    .hold_down_sights       = 0,
    .chat_shadow            = 1,
    .player_arms            = 0,
    .scale                  = 0,
    .tracing_enabled        = 0,
    .trajectory_length      = 16,
    .projectile_count       = 8,
    .show_minimap           = 1,
    .toggle_crouch          = 0,
    .toggle_sprint          = 0,
    .enable_shadows         = 1,
    .enable_particles       = 1,
    .smooth_orientation     = 1,
    .map_cache              = 0,
    .chat_beep              = 0,
    .show_crosshair         = 1,
    .show_health            = 1,
    .show_ammo              = 1,
    .report_client_version  = 1,
    .left_handed            = 0,
    .kill_indicator         = 0,
    .persistent_block_color = 0
};

char * config_filepath = "config.ini";

Setting * config_settings_begin = &config_settings[0];
Setting * config_settings_end = &config_settings[lengthof(config_settings)];

Options settings_tmp = {0};

List config_file, config_keybind;

ConfigKey * config_key(int key) {
    return &_config_key[key];
}

static void config_keys_update() {
    config_key(WINDOW_KEY_CROUCH)->toggle = settings.toggle_crouch;
    config_key(WINDOW_KEY_SPRINT)->toggle = settings.toggle_sprint;
}

static void config_sets(const char * section, const char * name, const char * value) {
    for (int k = 0; k < list_size(&config_file); k++) {
        ConfigFileEntry * e = list_get(&config_file, k);
        if (strcmp(e->name, name) == 0) {
            strncpy(e->value, value, sizeof(e->value) - 1);
            return;
        }
    }

    ConfigFileEntry e;
    strncpy(e.section, section, sizeof(e.section) - 1);
    strncpy(e.name,    name,    sizeof(e.name)    - 1);
    strncpy(e.value,   value,   sizeof(e.value)   - 1);
    list_add(&config_file, &e);
}

static void config_seti(const char * section, const char * name, int value) {
    char tmp[32];
    sprintf(tmp, "%i", value);
    config_sets(section, name, tmp);
}

static void config_setf(const char * section, const char * name, float value) {
    char tmp[32];
    sprintf(tmp, "%0.6f", value);
    config_sets(section, name, tmp);
}

void config_save() {
    config_keys_update();
    kv6_rebuild_complete();

    for (Setting * e = config_settings_begin; e != config_settings_end; e++) switch (e->type) {
        case CONFIG_TYPE_INT: config_seti("client", e->name, *((int *) e->value)); break;
        case CONFIG_TYPE_FLOAT: config_setf("client", e->name, *((float *) e->value)); break;
        case CONFIG_TYPE_STRING: config_sets("client", e->name, (const char *) e->value); break;
    }

    for (WindowKey key = WINDOW_KEY_FIRST; key <= WINDOW_KEY_LAST; key++) {
        ConfigKey * e = config_key(key);

        if (e->name != NULL) config_seti("controls", e->name, e->keycode);
    }

    void * fin = file_open(config_filepath, "w");

    if (fin != NULL) {
        char last_section[32] = {0};
        for (int k = 0; k < list_size(&config_file); k++) {
            ConfigFileEntry * e = list_get(&config_file, k);
            if (strcmp(e->section, last_section) != 0) {
                file_printf(fin, "\r\n[%s]\r\n", e->section);
                strcpy(last_section, e->section);
            }

            file_printf(fin, "%s", e->name);

            for (int l = 0; l < 31 - strlen(e->name); l++)
                file_printf(fin, " ");

            file_printf(fin, "= %s\r\n", e->value);
        }

        file_printf(fin, "\r\n[keybind]\r\n");
        for (int k = 0; k < list_size(&config_keybind); k++) {
            Keybind * keybind = list_get(&config_keybind, k);

            if (keybind->key > 0 && strlen(keybind->value) > 0)
                file_printf(fin, "%d = %s\r\n", keybind->key, keybind->value);
        }

        file_close(fin);
    }
}

static int config_read_key(void * user, const char * section, const char * name, const char * value) {
    if (strcmp(section, "keybind") == 0) {
        Keybind * keybind = list_add(&config_keybind, NULL);
        keybind->key = atoi(name);
        strncpy(keybind->value, value, sizeof(keybind->value));
    } else {
        ConfigFileEntry e;
        strncpy(e.section, section, sizeof(e.section) - 1);
        strncpy(e.name, name, sizeof(e.name) - 1);
        strncpy(e.value, value, sizeof(e.value) - 1);
        list_add(&config_file, &e);
    }

    if (strcmp(section, "client") == 0) {
        for (Setting * e = config_settings_begin; e != config_settings_end; e++) {
            if (strcmp(name, e->name) == 0) {
                switch (e->type) {
                    case CONFIG_TYPE_INT: *((int *) e->value) = atoi(value); break;
                    case CONFIG_TYPE_FLOAT: *((float *) e->value) = atof(value); break;
                    case CONFIG_TYPE_STRING: strcpy((char *) e->value, (const char *) value); break;
                }

                break;
            }
        }
    }

    if (strcmp(section, "controls") == 0) {
        for (WindowKey key = WINDOW_KEY_FIRST; key <= WINDOW_KEY_LAST; key++) {
            ConfigKey * e = config_key(key);

            if (strcmp(name, e->name) == 0) {
                log_debug("found override for %s, from %i to %i", e->name, e->keycode, atoi(value));
                e->keycode = strtol(value, NULL, 0);
                break;
            }
        }
    }

    return 1;
}

void config_reload() {
    memcpy(&settings_tmp, &settings, sizeof(Options));
    {
        if (!list_created(&config_keybind))
            list_create(&config_keybind, sizeof(Keybind));
        else
            list_clear(&config_keybind);

        if (!list_created(&config_file))
            list_create(&config_file, sizeof(ConfigFileEntry));
        else
            list_clear(&config_file);

        char * fin = (char *) file_load(config_filepath);

        if (fin != NULL) {
            ini_parse_string(fin, config_read_key, NULL);
            free(fin);
        }
    }
    memcpy(&settings, &settings_tmp, sizeof(Options));

    config_keys_update();

    settings.volume = clamp(0, 10, settings.volume);
    sound_volume(settings.volume / 10.0F);

    settings.camera_fov = clamp(CAMERA_DEFAULT_FOV, CAMERA_MAX_FOV, settings.camera_fov);
}

void config_init() {
    for (WindowKey key = WINDOW_KEY_FIRST; key <= WINDOW_KEY_LAST; key++)
        _config_key[key].original = _config_key[key].keycode;

    config_reload();
}
