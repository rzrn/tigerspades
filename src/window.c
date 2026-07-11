/*
    Copyright © 2017–2020, 2022 ByteBit
    Copyright © 2022 Julius C. Enriquez
    Copyright © 2023–2025 rzrn

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

#include <bs/common.h>
#include <bs/main.h>
#include <bs/window.h>
#include <bs/config.h>
#include <bs/hud.h>

#ifdef OS_WINDOWS
    #include <sysinfoapi.h>
    #include <windows.h>
#endif

#ifdef OS_LINUX
    #include <sys/sysinfo.h>
    #include <unistd.h>
#endif

#if defined(OS_FREEBSD) || defined(OS_OPENBSD) || defined(OS_NETBSD)
    #include <unistd.h>
#endif

#ifdef OS_HAIKU
    #include <kernel/OS.h>
#endif

void window_title(const char * suffix) {
    char title[128];

    if (suffix != NULL)
        snprintf(title, sizeof(title) - 1, "TigerSpades %s — %s (%s)", BSVERSION, suffix, TOOLKIT);
    else
        sprintf(title, "TigerSpades %s (%s)", BSVERSION, TOOLKIT);

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

void window_key_reset_togglestates(void) {
    for (WindowKey key = WINDOW_KEY_FIRST; key <= WINDOW_KEY_LAST; key++) {
        ConfigKey * e = config_key(key);

        if (e->toggle) window_pressed_keys[key] = 0;
    }
}

int window_cpucores(void) {
    #ifdef OS_LINUX
        #ifdef USE_TOUCH
            return sysconf(_SC_NPROCESSORS_CONF);
        #else
            return get_nprocs();
        #endif
    #endif

    #if defined(OS_FREEBSD) || defined(OS_OPENBSD) || defined(OS_NETBSD)
        return sysconf(_SC_NPROCESSORS_CONF);
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
    int count = 0;

    for (WindowKey key = WINDOW_KEY_FIRST; key <= WINDOW_KEY_LAST; key++) {
        if (config_key(key)->keycode == keycode) {
            sendkey(keycode, key, action, mod);
            count++;
        }
    }

    if (count == 0) sendkey(keycode, WINDOW_KEY_UNKNOWN, action, mod);
}

float window_aspect(void) {
    float w = settings.window_width;
    float h = settings.window_height;

    return w / h;
}
