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
#include <string.h>

#include <BetterSpades/common.h>
#include <BetterSpades/main.h>
#include <BetterSpades/window.h>
#include <BetterSpades/config.h>
#include <BetterSpades/hud.h>

#ifdef OS_WINDOWS
    #include <sysinfoapi.h>
    #include <windows.h>
#endif

#ifdef OS_LINUX
    #include <sys/sysinfo.h>
    #include <unistd.h>
#endif

#ifdef OS_HAIKU
    #include <kernel/OS.h>
#endif

void window_title(const char * suffix) {
    char title[128];

    if (suffix)
        snprintf(title, sizeof(title) - 1, "TigerSpades %s — %s (%s)", BETTERSPADES_VERSION, suffix, TOOLKIT);
    else
        sprintf(title, "TigerSpades %s (%s)", BETTERSPADES_VERSION, TOOLKIT);

    window_settitle(title);
}

void window_setmouseloc(double x, double y) {
    UNUSED(x);
    UNUSED(y);
}

#define WINDOW_KEY_TOTAL (WINDOW_KEY_LAST + 1)
int window_pressed_keys[WINDOW_KEY_TOTAL] = {0};

int window_key_down(int key) {
    return window_pressed_keys[key] > 0;
}

void window_key_reset_togglestates() {
    for (int k = 0; k < list_size(&config_keys); k++) {
        ConfigKeyPair * a = list_get(&config_keys, k);
        if (a->toggle) window_pressed_keys[a->internal] = 0;
    }
}

int window_cpucores() {
    #ifdef OS_LINUX
        #ifdef USE_TOUCH
            return sysconf(_SC_NPROCESSORS_CONF);
        #else
            return get_nprocs();
        #endif
    #endif

    #ifdef OS_WINDOWS
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return info.dwNumberOfProcessors;
    #endif

    #ifdef OS_HAIKU
        system_info info;
        get_system_info(&info);
        return info.cpu_count;
    #endif

    return 1;
}

static inline void sendkey(int keycode, int key, int action, int mod) {
    if (key != WINDOW_KEY_UNKNOWN) keys(key, action, mod);

    if (hud_active->input_keyboard != NULL)
        hud_active->input_keyboard(key, action, mod, keycode);
}

void window_sendkey(int action, int keycode, int mod) {
    unsigned int count = 0;

    for (int k = 0; k < list_size(&config_keys); k++) {
        ConfigKeyPair * a = list_get(&config_keys, k);

        if (a->def == keycode) {
            sendkey(keycode, a->internal, action, mod);
            count++;
        }
    }

    if (count == 0) sendkey(keycode, WINDOW_KEY_UNKNOWN, action, mod);
}
