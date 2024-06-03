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
#include <BetterSpades/gui.h>

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
}

#define WINDOW_KEY_TOTAL (WINDOW_KEY_LAST + 1)
int window_pressed_keys[WINDOW_KEY_TOTAL] = {0};

int window_key_down(int key) {
    return window_pressed_keys[key] > 0;
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

void window_sendkey(int action, int keycode, int mod) {
    int count = config_key_translate(keycode, 0, NULL);

    if (count > 0) {
        int results[count];
        config_key_translate(keycode, 0, results);
        for (int k = 0; k < count; k++) {
            keys(hud_window, results[k], action, mod);
            if (hud_active->input_keyboard)
                hud_active->input_keyboard(results[k], action, mod, keycode);
        }
    } else {
        if (hud_active->input_keyboard)
            hud_active->input_keyboard(WINDOW_KEY_UNKNOWN, action, mod, keycode);
    }
}
