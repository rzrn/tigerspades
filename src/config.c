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

#include <BetterSpades/window.h>
#include <BetterSpades/file.h>
#include <BetterSpades/config.h>
#include <BetterSpades/sound.h>
#include <BetterSpades/model.h>
#include <BetterSpades/camera.h>
#include <BetterSpades/utils.h>
#include <BetterSpades/gui.h>

#include <ini.h>

char * config_filepath = "config.ini";

Options settings = {
    .opengl14           = 1,
    .color_correction   = 0,
    .shadow_entities    = 0,
    .render_distance    = RENDER_DISTANCE,
    .name               = "Deuce",
    .window_width       = 800,
    .window_height      = 600,
    .min_lan_port       = 32882,
    .max_lan_port       = 32892,
    .volume             = 10,
    .invert_y           = 0,
    .fullscreen         = 0,
    .mouse_sensitivity  = 5.0F,
    .show_news          = 1,
    .multisamples       = 0,
    .greedy_meshing     = 0,
    .vsync              = 1,
    .voxlap_models      = 0,
    .force_displaylist  = 0,
    .smooth_fog         = 0,
    .ambient_occlusion  = 0,
    .camera_fov         = CAMERA_DEFAULT_FOV,
    .hold_down_sights   = 0,
    .chat_shadow        = 1,
    .player_arms        = 0,
    .scale              = 0,
    .tracing_enabled    = 0,
    .trajectory_length  = 16,
    .projectile_count   = 8,
    .show_minimap       = 1,
    .toggle_crouch      = 0,
    .toggle_sprint      = 0,
    .enable_shadows     = 1,
    .enable_particles   = 1,
    .smooth_orientation = 1,
    .map_cache          = 0,
};

Options settings_tmp = {0};

List config_keys, config_settings, config_file, config_keybind;

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

    config_sets("client", "name",               settings.name);
    config_seti("client", "min_lan_port",       settings.min_lan_port);
    config_seti("client", "max_lan_port",       settings.max_lan_port);
    config_seti("client", "xres",               settings.window_width);
    config_seti("client", "yres",               settings.window_height);
    config_seti("client", "windowed",           !settings.fullscreen);
    config_seti("client", "multisamples",       settings.multisamples);
    config_seti("client", "greedy_meshing",     settings.greedy_meshing);
    config_seti("client", "vsync",              settings.vsync);
    config_setf("client", "mouse_sensitivity",  settings.mouse_sensitivity);
    config_seti("client", "show_news",          settings.show_news);
    config_seti("client", "vol",                settings.volume);
    config_seti("client", "voxlap_models",      settings.voxlap_models);
    config_seti("client", "force_displaylist",  settings.force_displaylist);
    config_seti("client", "inverty",            settings.invert_y);
    config_seti("client", "smooth_fog",         settings.smooth_fog);
    config_seti("client", "ambient_occlusion",  settings.ambient_occlusion);
    config_setf("client", "camera_fov",         settings.camera_fov);
    config_seti("client", "hold_down_sights",   settings.hold_down_sights);
    config_seti("client", "chat_shadow",        settings.chat_shadow);
    config_seti("client", "show_player_arms",   settings.player_arms);
    config_seti("client", "scale",              settings.scale);
    config_seti("client", "tracing_enabled",    settings.tracing_enabled);
    config_seti("client", "trajectory_length",  settings.trajectory_length);
    config_seti("client", "projectile_count",   settings.projectile_count);
    config_seti("client", "show_minimap",       settings.show_minimap);
    config_seti("client", "toggle_crouch",      settings.toggle_crouch);
    config_seti("client", "toggle_sprint",      settings.toggle_sprint);
    config_seti("client", "enable_shadows",     settings.enable_shadows);
    config_seti("client", "enable_particles",   settings.enable_particles);
    config_seti("client", "smooth_orientation", settings.smooth_orientation);
    config_seti("client", "map_cache",          settings.map_cache);

    for (int k = 0; k < list_size(&config_keys); k++) {
        ConfigKeyPair * e = list_get(&config_keys, k);
        if (strlen(e->name) > 0)
            config_seti("controls", e->name, e->def);
    }

    void * fin = file_open(config_filepath, "w");
    if (fin) {
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
    if (strcmp(section, "keybind") != 0) {
        ConfigFileEntry e;
        strncpy(e.section, section, sizeof(e.section) - 1);
        strncpy(e.name, name, sizeof(e.name) - 1);
        strncpy(e.value, value, sizeof(e.value) - 1);
        list_add(&config_file, &e);
    } else {
        Keybind * keybind = list_add(&config_keybind, NULL);
        keybind->key = atoi(name);
        strncpy(keybind->value, value, sizeof(keybind->value));
    }

    if (!strcmp(section, "client")) {
        if (!strcmp(name, "name")) {
            strcpy(settings.name, value);
        } else if (!strcmp(name, "min_lan_port")) {
            settings.min_lan_port = atoi(value);
        } else if (!strcmp(name, "max_lan_port")) {
            settings.max_lan_port = atoi(value);
        } else if (!strcmp(name, "xres")) {
            settings.window_width = atoi(value);
        } else if (!strcmp(name, "yres")) {
            settings.window_height = atoi(value);
        } else if (!strcmp(name, "windowed")) {
            settings.fullscreen = !atoi(value);
        } else if (!strcmp(name, "multisamples")) {
            settings.multisamples = atoi(value);
        } else if (!strcmp(name, "greedy_meshing")) {
            settings.greedy_meshing = atoi(value);
        } else if (!strcmp(name, "vsync")) {
            settings.vsync = atoi(value);
        } else if (!strcmp(name, "mouse_sensitivity")) {
            settings.mouse_sensitivity = atof(value);
        } else if (!strcmp(name, "show_news")) {
            settings.show_news = atoi(value);
        } else if (!strcmp(name, "vol")) {
            settings.volume = clamp(0, 10, atoi(value));
            sound_volume(settings.volume / 10.0F);
        } else if (!strcmp(name, "voxlap_models")) {
            settings.voxlap_models = atoi(value);
        } else if (!strcmp(name, "force_displaylist")) {
            settings.force_displaylist = atoi(value);
        } else if (!strcmp(name, "inverty")) {
            settings.invert_y = atoi(value);
        } else if (!strcmp(name, "smooth_fog")) {
            settings.smooth_fog = atoi(value);
        } else if (!strcmp(name, "ambient_occlusion")) {
            settings.ambient_occlusion = atoi(value);
        } else if (!strcmp(name, "camera_fov")) {
            settings.camera_fov = fmax(fmin(atof(value), CAMERA_MAX_FOV), CAMERA_DEFAULT_FOV);
        } else if (!strcmp(name, "hold_down_sights")) {
            settings.hold_down_sights = atoi(value);
        } else if (!strcmp(name, "chat_shadow")) {
            settings.chat_shadow = atoi(value);
        } else if (!strcmp(name, "show_player_arms")) {
            settings.player_arms = atoi(value);
        } else if (!strcmp(name, "scale")) {
            settings.scale = atoi(value);
        } else if (!strcmp(name, "tracing_enabled")) {
            settings.tracing_enabled = atoi(value);
        } else if (!strcmp(name, "trajectory_length")) {
            settings.trajectory_length = atoi(value);
        } else if (!strcmp(name, "projectile_count")) {
            settings.projectile_count = atoi(value);
        } else if (!strcmp(name, "show_minimap")) {
            settings.show_minimap = atoi(value);
        } else if (!strcmp(name, "toggle_crouch")) {
            settings.toggle_crouch = atoi(value);
        } else if (!strcmp(name, "toggle_sprint")) {
            settings.toggle_sprint = atoi(value);
        } else if (!strcmp(name, "enable_shadows")) {
            settings.enable_shadows = atoi(value);
        } else if (!strcmp(name, "enable_particles")) {
            settings.enable_particles = atoi(value);
        } else if (!strcmp(name, "smooth_orientation")) {
            settings.smooth_orientation = atoi(value);
        } else if (!strcmp(name, "map_cache")) {
            settings.map_cache = atoi(value);
        }
    }

    if (!strcmp(section, "controls")) {
        for (int k = 0; k < list_size(&config_keys); k++) {
            ConfigKeyPair * key = list_get(&config_keys, k);

            if (!strcmp(name, key->name)) {
                log_debug("found override for %s, from %i to %i", key->name, key->def, atoi(value));
                key->def = strtol(value, NULL, 0);
                break;
            }
        }
    }

    return 1;
}

ConfigKeyPair * config_key(int key) {
    for (int k = 0; k < list_size(&config_keys); k++) {
        ConfigKeyPair * a = list_get(&config_keys, k);
        if (a->internal == key)
            return a;
    }

    return NULL;
}

static void config_label_scale(char * buffer, size_t length, int value, size_t index) {
    if (value == 0) {
        snprintf(buffer, length, "Auto");
    } else {
        snprintf(buffer, length, "%i", value);
    }
}

static void config_label_pixels(char * buffer, size_t length, int value, size_t index) {
    if (value == 800 || value == 600) {
        snprintf(buffer, length, "default: %ipx", value);
    } else {
        snprintf(buffer, length, "%ipx", value);
    }
}

static void config_label_vsync(char * buffer, size_t length, int value, size_t index) {
    if (value == 0) {
        snprintf(buffer, length, "disabled");
    } else if (value == 1) {
        snprintf(buffer, length, "enabled");
    } else {
        snprintf(buffer, length, "max %i fps", value);
    }
}

static void config_label_msaa(char * buffer, size_t length, int value, size_t index) {
    if (index == 0) {
        snprintf(buffer, length, "No MSAA");
    } else {
        snprintf(buffer, length, "%ix MSAA", value);
    }
}

static const char * config_key_category = NULL;
#define CATEGORY(x) { config_key_category = (x); }

static void config_register_key(int internal, int def, const char * name, int toggle, const char * display) {
    ConfigKeyPair key;
    key.internal = internal;
    key.def      = def;
    key.original = def;
    key.toggle   = toggle;

    if (display)
        strncpy(key.display, display, sizeof(key.display) - 1);
    else
        *key.display = 0;

    if (name)
        strncpy(key.name, name, sizeof(key.name) - 1);
    else
        *key.name = 0;

    if (config_key_category)
        strncpy(key.category, config_key_category, sizeof(key.category) - 1);
    else
        *key.category = 0;

    list_add(&config_keys, &key);
}

void config_reload() {
    if (!list_created(&config_keybind))
        list_create(&config_keybind, sizeof(Keybind));
    else
        list_clear(&config_keybind);

    if (!list_created(&config_file))
        list_create(&config_file, sizeof(ConfigFileEntry));
    else
        list_clear(&config_file);

    if (!list_created(&config_keys))
        list_create(&config_keys, sizeof(ConfigKeyPair));
    else
        list_clear(&config_keys);

    CATEGORY("Movement") {
        config_register_key(WINDOW_KEY_UP,           TOOLKIT_KEY_W,            "move_forward",      0, "Forward");
        config_register_key(WINDOW_KEY_LEFT,         TOOLKIT_KEY_A,            "move_left",         0, "Left");
        config_register_key(WINDOW_KEY_DOWN,         TOOLKIT_KEY_S,            "move_backward",     0, "Backward");
        config_register_key(WINDOW_KEY_RIGHT,        TOOLKIT_KEY_D,            "move_right",        0, "Right");
        config_register_key(WINDOW_KEY_SPACE,        TOOLKIT_KEY_SPACE,        "jump",              0, "Jump");
        config_register_key(WINDOW_KEY_SPRINT,       TOOLKIT_KEY_SHIFT,        "sprint",            0, "Sprint");
        config_register_key(WINDOW_KEY_CROUCH,       TOOLKIT_KEY_CONTROL,      "crouch",            0, "Crouch");
        config_register_key(WINDOW_KEY_SNEAK,        TOOLKIT_KEY_V,            "sneak",             0, "Sneak");
    }

    CATEGORY("Block") {
        config_register_key(WINDOW_KEY_CURSOR_UP,    TOOLKIT_KEY_CURSOR_UP,    "cube_color_up",     0, "Color up");
        config_register_key(WINDOW_KEY_CURSOR_DOWN,  TOOLKIT_KEY_CURSOR_DOWN,  "cube_color_down",   0, "Color down");
        config_register_key(WINDOW_KEY_CURSOR_LEFT,  TOOLKIT_KEY_CURSOR_LEFT,  "cube_color_left",   0, "Color left");
        config_register_key(WINDOW_KEY_CURSOR_RIGHT, TOOLKIT_KEY_CURSOR_RIGHT, "cube_color_right",  0, "Color right");
        config_register_key(WINDOW_KEY_PICKCOLOR,    TOOLKIT_KEY_E,            "cube_color_sample", 0, "Pick color");
    }

    CATEGORY("Tools & Weapons") {
        config_register_key(WINDOW_KEY_TOOL1,        TOOLKIT_KEY_1,            "tool_spade",        0, "Select spade");
        config_register_key(WINDOW_KEY_TOOL2,        TOOLKIT_KEY_2,            "tool_block",        0, "Select block");
        config_register_key(WINDOW_KEY_TOOL3,        TOOLKIT_KEY_3,            "tool_gun",          0, "Select gun");
        config_register_key(WINDOW_KEY_TOOL4,        TOOLKIT_KEY_4,            "tool_grenade",      0, "Select grenade");
        config_register_key(WINDOW_KEY_RELOAD,       TOOLKIT_KEY_R,            "reload",            0, "Reload");
        config_register_key(WINDOW_KEY_CHANGEWEAPON, TOOLKIT_KEY_PERIOD,       "change_weapon",     0, "Gun select");
        config_register_key(WINDOW_KEY_LASTTOOL,     TOOLKIT_KEY_Q,            "last_tool",         0, "Last tool");
    }

    CATEGORY("Game") {
        config_register_key(WINDOW_KEY_ESCAPE,       TOOLKIT_KEY_ESCAPE,       "quit_game",         0, "Quit");
        config_register_key(WINDOW_KEY_VOLUME_UP,    TOOLKIT_KEY_ADD,          "volume_up",         0, "Volume up");
        config_register_key(WINDOW_KEY_VOLUME_DOWN,  TOOLKIT_KEY_SUBTRACT,     "volume_down",       0, "Volume down");
        config_register_key(WINDOW_KEY_CHAT,         TOOLKIT_KEY_T,            "chat_global",       0, "Chat");
        config_register_key(WINDOW_KEY_TEAM_CHAT,    TOOLKIT_KEY_Y,            "chat_team",         0, "Team chat");
        config_register_key(WINDOW_KEY_FULLSCREEN,   TOOLKIT_KEY_F11,          "fullscreen",        0, "Fullscreen");
        config_register_key(WINDOW_KEY_SCREENSHOT,   TOOLKIT_KEY_F5,           "screenshot",        0, "Screenshot");
        config_register_key(WINDOW_KEY_CHANGETEAM,   TOOLKIT_KEY_COMMA,        "change_team",       0, "Team select");
        config_register_key(WINDOW_KEY_COMMAND,      TOOLKIT_KEY_SLASH,        "chat_command",      0, "Command");
        config_register_key(WINDOW_KEY_HIDEHUD,      TOOLKIT_KEY_F6,           "hide_hud",          1, "Hide HUD");
        config_register_key(WINDOW_KEY_SAVE_MAP,     TOOLKIT_KEY_F9,           "save_map",          0, "Save map");
    }

    CATEGORY("Information") {
        config_register_key(WINDOW_KEY_TAB,          TOOLKIT_KEY_TAB,          "view_score",        0, "Score");
        config_register_key(WINDOW_KEY_MAP,          TOOLKIT_KEY_M,            "view_map",          1, "Map");
        config_register_key(WINDOW_KEY_NETWORKSTATS, TOOLKIT_KEY_F12,          "network_stats",     1, "Network stats");
        config_register_key(WINDOW_KEY_DEBUG,        TOOLKIT_KEY_F3,           "debug",             1, "Debug screen");
        config_register_key(WINDOW_KEY_TRACE_CLEAN,  TOOLKIT_KEY_F4,           "trace_clean",       0, "Clean up bullets");
    }

    CATEGORY("Local game") {
        config_register_key(WINDOW_KEY_CYCLE_CAMERA, TOOLKIT_KEY_F2,           "cycle_camera",      0, "Change camera mode");
        config_register_key(WINDOW_KEY_TOGGLE_ALIVE, TOOLKIT_KEY_F4,           "toggle_alive",      0, "Toggle aliveness");
        config_register_key(WINDOW_KEY_RESPAWN,      TOOLKIT_KEY_F7,           "respawn",           0, "Respawn");
        config_register_key(WINDOW_KEY_RESTOCK,      TOOLKIT_KEY_F8,           "restock",           0, "Restock");
        config_register_key(WINDOW_KEY_TEAM_COLOR,   TOOLKIT_KEY_F10,          "team_color",        0, "Change team color");
    }

    CATEGORY(NULL) {
        config_register_key(WINDOW_KEY_SHIFT,        TOOLKIT_KEY_SHIFT,        NULL,                0, NULL);
        config_register_key(WINDOW_KEY_BACKSPACE,    TOOLKIT_KEY_BACKSPACE,    NULL,                0, NULL);
        config_register_key(WINDOW_KEY_ENTER,        TOOLKIT_KEY_ENTER,        NULL,                0, NULL);
        config_register_key(WINDOW_KEY_V,            TOOLKIT_KEY_V,            NULL,                0, NULL);
        config_register_key(WINDOW_KEY_SELECT1,      TOOLKIT_KEY_1,            NULL,                0, NULL);
        config_register_key(WINDOW_KEY_SELECT2,      TOOLKIT_KEY_2,            NULL,                0, NULL);
        config_register_key(WINDOW_KEY_SELECT3,      TOOLKIT_KEY_3,            NULL,                0, NULL);
    }

    char * fin = (char *) file_load(config_filepath);

    if (fin) {
        ini_parse_string(fin, config_read_key, NULL);
        free(fin);
    }

    config_keys_update();

    if (!list_created(&config_settings))
        list_create(&config_settings, sizeof(Setting));
    else
        list_clear(&config_settings);

    list_add(&config_settings,
             &(Setting) {
                 .value    = settings_tmp.name,
                 .type     = CONFIG_TYPE_STRING,
                 .max      = sizeof(settings.name) - 1,
                 .name     = "Name",
                 .help     = "Ingame player name",
                 .category = "Game"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.show_minimap,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .name     = "Show minimap",
                 .category = "Game"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.min_lan_port,
                 .type     = CONFIG_TYPE_INT,
                 .max      = INT_MAX,
                 .name     = "Minimum LAN port",
                 .help     = "First port to scan for LAN games",
                 .category = "Game"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.max_lan_port,
                 .type     = CONFIG_TYPE_INT,
                 .max      = INT_MAX,
                 .name     = "Maximum LAN port",
                 .help     = "Last port to scan for LAN games",
                 .category = "Game"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.map_cache,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .name     = "Use map cache",
                 .help     = "Can use a lot of disk space",
                 .category = "Game"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.mouse_sensitivity,
                 .type     = CONFIG_TYPE_FLOAT,
                 .min      = 0,
                 .max      = INT_MAX,
                 .name     = "Mouse sensitivity",
                 .category = "Control"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.invert_y,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .name     = "Invert Y",
                 .help     = "Invert vertical mouse movement",
                 .category = "Control"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.hold_down_sights,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Only aim while pressing RMB",
                 .name     = "Hold down sights",
                 .category = "Control"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.toggle_crouch,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .name     = "Toggle crouch",
                 .category = "Control"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.toggle_sprint,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .name     = "Toggle sprint",
                 .category = "Control"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.volume,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 10,
                 .name     = "Volume",
                 .category = "Interface"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value           = &settings_tmp.scale,
                 .type            = CONFIG_TYPE_INT,
                 .min             = 0,
                 .max             = INT_MAX,
                 .name            = "GUI scale",
                 .category        = "Interface",
                 .defaults        = {0, 1, 2, 4, 8, 16, 32, 64},
                 .defaults_length = 8,
                 .label_callback  = config_label_scale
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.chat_shadow,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Dark chat background",
                 .name     = "Chat shadow",
                 .category = "Interface"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.show_news,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .name     = "Show news",
                 .help     = "Show news on server list",
                 .category = "Interface"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.camera_fov,
                 .type     = CONFIG_TYPE_FLOAT,
                 .min      = CAMERA_DEFAULT_FOV,
                 .max      = CAMERA_MAX_FOV,
                 .name     = "Camera FOV",
                 .help     = "Field of View in degrees",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value           = &settings_tmp.window_width,
                 .type            = CONFIG_TYPE_INT,
                 .min             = 0,
                 .max             = INT_MAX,
                 .name            = "Game width",
                 .defaults        = {640, 800, 854, 1024, 1280, 1920, 3840},
                 .defaults_length = 7,
                 .help            = "Default: 800",
                 .category        = "Graphics",
                 .label_callback  = config_label_pixels
             });
    list_add(&config_settings,
             &(Setting) {
                 .value           = &settings_tmp.window_height,
                 .type            = CONFIG_TYPE_INT,
                 .min             = 0,
                 .max             = INT_MAX,
                 .name            = "Game height",
                 .defaults        = {480, 600, 720, 768, 1024, 1080, 2160},
                 .defaults_length = 7,
                 .help            = "Default: 600",
                 .category        = "Graphics",
                 .label_callback  = config_label_pixels
             });
    list_add(&config_settings,
             &(Setting) {
                 .value           = &settings_tmp.vsync,
                 .type            = CONFIG_TYPE_INT,
                 .min             = 0,
                 .max             = INT_MAX,
                 .name            = "V-Sync",
                 .help            = "Limits your game's fps",
                 .category        = "Graphics",
                 .defaults        = {0, 1, 20, 30, 60, 120, 144, 240},
                 .defaults_length = 8,
                 .label_callback  = config_label_vsync
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.fullscreen,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .name     = "Fullscreen",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value           = &settings_tmp.multisamples,
                 .type            = CONFIG_TYPE_INT,
                 .min             = 0,
                 .max             = 16,
                 .name            = "Multisamples",
                 .help            = "Smooth out block edges",
                 .category        = "Graphics",
                 .defaults        = {0, 2, 4, 8, 16},
                 .defaults_length = 5,
                 .label_callback  = config_label_msaa
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.voxlap_models,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Render models like in voxlap",
                 .name     = "Voxlap models",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.greedy_meshing,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Join similar mesh faces",
                 .name     = "Greedy meshing",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.force_displaylist,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Enable this on buggy drivers",
                 .name     = "Force Displaylist",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.smooth_fog,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Enable this on buggy drivers",
                 .name     = "Smooth fog",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.ambient_occlusion,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "(won't work with greedy mesh)",
                 .name     = "Ambient occlusion",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.enable_particles,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Disable this on weak hardware",
                 .name     = "Enable particles",
                 .category = "Graphics"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.tracing_enabled,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Requires server support",
                 .name     = "Bullet tracing",
                 .category = "Debug"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.trajectory_length,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 16,
                 .max      = 2048,
                 .name     = "Trajectory length",
                 .category = "Debug"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.projectile_count,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 8,
                 .max      = 256,
                 .name     = "Projectile count",
                 .category = "Debug"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.enable_shadows,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Useful for map development",
                 .name     = "Map shadows",
                 .category = "Debug"
             });
    list_add(&config_settings,
             &(Setting) {
                 .value    = &settings_tmp.smooth_orientation,
                 .type     = CONFIG_TYPE_INT,
                 .min      = 0,
                 .max      = 1,
                 .help     = "Disable to spectate cheaters",
                 .name     = "Smooth orientation",
                 .category = "Debug"
             });
}
