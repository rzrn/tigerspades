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
#include <unistd.h>
#endif

#ifdef OS_HAIKU
#include <kernel/OS.h>
#endif

#ifdef USE_GLFW
    const char * glfw_get_fnkey_name(int keycode) {
        switch (keycode) {
            case GLFW_KEY_A:             return "A";
            case GLFW_KEY_B:             return "B";
            case GLFW_KEY_C:             return "C";
            case GLFW_KEY_D:             return "D";
            case GLFW_KEY_E:             return "E";
            case GLFW_KEY_F:             return "F";
            case GLFW_KEY_G:             return "G";
            case GLFW_KEY_H:             return "H";
            case GLFW_KEY_I:             return "I";
            case GLFW_KEY_J:             return "J";
            case GLFW_KEY_K:             return "K";
            case GLFW_KEY_L:             return "L";
            case GLFW_KEY_M:             return "M";
            case GLFW_KEY_N:             return "N";
            case GLFW_KEY_O:             return "O";
            case GLFW_KEY_P:             return "P";
            case GLFW_KEY_Q:             return "Q";
            case GLFW_KEY_R:             return "R";
            case GLFW_KEY_S:             return "S";
            case GLFW_KEY_T:             return "T";
            case GLFW_KEY_U:             return "U";
            case GLFW_KEY_V:             return "V";
            case GLFW_KEY_W:             return "W";
            case GLFW_KEY_X:             return "X";
            case GLFW_KEY_Y:             return "Y";
            case GLFW_KEY_Z:             return "Z";
            case GLFW_KEY_1:             return "1";
            case GLFW_KEY_2:             return "2";
            case GLFW_KEY_3:             return "3";
            case GLFW_KEY_4:             return "4";
            case GLFW_KEY_5:             return "5";
            case GLFW_KEY_6:             return "6";
            case GLFW_KEY_7:             return "7";
            case GLFW_KEY_8:             return "8";
            case GLFW_KEY_9:             return "9";
            case GLFW_KEY_0:             return "0";
            case GLFW_KEY_SPACE:         return "Space";
            case GLFW_KEY_MINUS:         return "-";
            case GLFW_KEY_EQUAL:         return "=";
            case GLFW_KEY_LEFT_BRACKET:  return "(";
            case GLFW_KEY_RIGHT_BRACKET: return ")";
            case GLFW_KEY_BACKSLASH:     return "\\";
            case GLFW_KEY_SEMICOLON:     return ":";
            case GLFW_KEY_APOSTROPHE:    return "'";
            case GLFW_KEY_GRAVE_ACCENT:  return "`";
            case GLFW_KEY_COMMA:         return ",";
            case GLFW_KEY_PERIOD:        return ".";
            case GLFW_KEY_SLASH:         return "/";
            case GLFW_KEY_ESCAPE:        return "Escape";
            case GLFW_KEY_F1:            return "F1";
            case GLFW_KEY_F2:            return "F2";
            case GLFW_KEY_F3:            return "F3";
            case GLFW_KEY_F4:            return "F4";
            case GLFW_KEY_F5:            return "F5";
            case GLFW_KEY_F6:            return "F6";
            case GLFW_KEY_F7:            return "F7";
            case GLFW_KEY_F8:            return "F8";
            case GLFW_KEY_F9:            return "F9";
            case GLFW_KEY_F10:           return "F10";
            case GLFW_KEY_F11:           return "F11";
            case GLFW_KEY_F12:           return "F12";
            case GLFW_KEY_F13:           return "F13";
            case GLFW_KEY_F14:           return "F14";
            case GLFW_KEY_F15:           return "F15";
            case GLFW_KEY_F16:           return "F16";
            case GLFW_KEY_F17:           return "F17";
            case GLFW_KEY_F18:           return "F18";
            case GLFW_KEY_F19:           return "F19";
            case GLFW_KEY_F20:           return "F20";
            case GLFW_KEY_F21:           return "F21";
            case GLFW_KEY_F22:           return "F22";
            case GLFW_KEY_F23:           return "F23";
            case GLFW_KEY_F24:           return "F24";
            case GLFW_KEY_F25:           return "F25";
            case GLFW_KEY_UP:            return "Up";
            case GLFW_KEY_DOWN:          return "Down";
            case GLFW_KEY_LEFT:          return "Left";
            case GLFW_KEY_RIGHT:         return "Right";
            case GLFW_KEY_LEFT_SHIFT:    return "Left Shift";
            case GLFW_KEY_RIGHT_SHIFT:   return "Right Shift";
            case GLFW_KEY_LEFT_CONTROL:  return "Left Ctrl";
            case GLFW_KEY_RIGHT_CONTROL: return "Right Ctrl";
            case GLFW_KEY_LEFT_ALT:      return "Left Alt";
            case GLFW_KEY_RIGHT_ALT:     return "Right Alt";
            case GLFW_KEY_TAB:           return "Tab";
            case GLFW_KEY_ENTER:         return "Enter";
            case GLFW_KEY_BACKSPACE:     return "Backspace";
            case GLFW_KEY_INSERT:        return "Insert";
            case GLFW_KEY_DELETE:        return "Delete";
            case GLFW_KEY_PAGE_UP:       return "Page Up";
            case GLFW_KEY_PAGE_DOWN:     return "Page Down";
            case GLFW_KEY_HOME:          return "Home";
            case GLFW_KEY_END:           return "End";
            case GLFW_KEY_KP_0:          return "Keypad 0";
            case GLFW_KEY_KP_1:          return "Keypad 1";
            case GLFW_KEY_KP_2:          return "Keypad 2";
            case GLFW_KEY_KP_3:          return "Keypad 3";
            case GLFW_KEY_KP_4:          return "Keypad 4";
            case GLFW_KEY_KP_5:          return "Keypad 5";
            case GLFW_KEY_KP_6:          return "Keypad 6";
            case GLFW_KEY_KP_7:          return "Keypad 7";
            case GLFW_KEY_KP_8:          return "Keypad 8";
            case GLFW_KEY_KP_9:          return "Keypad 9";
            case GLFW_KEY_KP_DIVIDE:     return "Keypad /";
            case GLFW_KEY_KP_MULTIPLY:   return "Keypad *";
            case GLFW_KEY_KP_SUBTRACT:   return "Keypad -";
            case GLFW_KEY_KP_ADD:        return "Keypad +";
            case GLFW_KEY_KP_DECIMAL:    return "Keypad Decimal";
            case GLFW_KEY_KP_EQUAL:      return "Keypad =";
            case GLFW_KEY_KP_ENTER:      return "Keypad Enter";
            case GLFW_KEY_PRINT_SCREEN:  return "Print Screen";
            case GLFW_KEY_NUM_LOCK:      return "Num Lock";
            case GLFW_KEY_CAPS_LOCK:     return "Caps Lock";
            case GLFW_KEY_SCROLL_LOCK:   return "Scroll Lock";
            case GLFW_KEY_PAUSE:         return "Pause";
            case GLFW_KEY_LEFT_SUPER:    return "Left Super";
            case GLFW_KEY_RIGHT_SUPER:   return "Right Super";
            case GLFW_KEY_MENU:          return "Menu";
            default:                     return NULL;
        }
    }
#endif

#ifdef USE_GLUT
    // FreeGLUT extension
    #define GLUT_KEY_NUM_LOCK  0x006D
    #define GLUT_KEY_BEGIN     0x006E
    #define GLUT_KEY_DELETE    0x006F
    #define GLUT_KEY_SHIFT_L   0x0070
    #define GLUT_KEY_SHIFT_R   0x0071
    #define GLUT_KEY_CTRL_L    0x0072
    #define GLUT_KEY_CTRL_R    0x0073
    #define GLUT_KEY_ALT_L     0x0074
    #define GLUT_KEY_ALT_R     0x0075

    char * glut_special_key_name(int keycode) {
        switch (keycode) {
            case GLUT_KEY_F1:        return "F1";
            case GLUT_KEY_F2:        return "F2";
            case GLUT_KEY_F3:        return "F3";
            case GLUT_KEY_F4:        return "F4";
            case GLUT_KEY_F5:        return "F5";
            case GLUT_KEY_F6:        return "F6";
            case GLUT_KEY_F7:        return "F7";
            case GLUT_KEY_F8:        return "F8";
            case GLUT_KEY_F9:        return "F9";
            case GLUT_KEY_F10:       return "F10";
            case GLUT_KEY_F11:       return "F11";
            case GLUT_KEY_F12:       return "F12";
            case GLUT_KEY_LEFT:      return "Left";
            case GLUT_KEY_UP:        return "Up";
            case GLUT_KEY_RIGHT:     return "Right";
            case GLUT_KEY_DOWN:      return "Down";
            case GLUT_KEY_PAGE_UP:   return "Page Up";
            case GLUT_KEY_PAGE_DOWN: return "Page Down";
            case GLUT_KEY_HOME:      return "Home";
            case GLUT_KEY_END:       return "End";
            case GLUT_KEY_INSERT:    return "Insert";
            case GLUT_KEY_NUM_LOCK:  return "Num Lock";
            case GLUT_KEY_BEGIN:     return "Home";
            case GLUT_KEY_DELETE:    return "Delete";
            case GLUT_KEY_SHIFT_L:   return "Left Shift";
            case GLUT_KEY_SHIFT_R:   return "Right Shift";
            case GLUT_KEY_CTRL_L:    return "Left Ctrl";
            case GLUT_KEY_CTRL_R:    return "Right Ctrl";
            case GLUT_KEY_ALT_L:     return "Left Alt";
            case GLUT_KEY_ALT_R:     return "Right Alt";
            default:                 return NULL;
        }
    }
#endif

void window_keyname(int keycode, char * output, size_t length) {
    if (keycode == 0) { output[0] = '?'; output[1] = 0; return; }

    #if defined(OS_WINDOWS) && defined(USE_GLFW)
        GetKeyNameTextA(glfwGetKeyScancode(keycode) << 16, output, length);
    #else
        #ifdef USE_GLFW
            const char * keyname = glfw_get_fnkey_name(keycode);

            if (keyname == NULL)
                keyname = glfwGetKeyName(keycode, 0);
        #endif

        #ifdef USE_SDL
            const char * keyname = SDL_GetKeyName(keycode);
        #endif

        #ifdef USE_GLUT
            char * keyname = NULL;

            if (keycode & GLUT_SPECIAL_MASK)
                keyname = glut_special_key_name(keycode & ~GLUT_SPECIAL_MASK);
            else switch (keycode) {
                case 0:  keyname = "Null";      break;
                case 7:  keyname = "Beep";      break;
                case 8:  keyname = "Backspace"; break;
                case 9:  keyname = "Tab";       break;
                case 10: keyname = "Enter";     break;
                case 13: keyname = "Enter";     break;
                case 27: keyname = "Escape";    break;
                case 32: keyname = "Space";     break;
                default: {
                    static char buf[2];
                    buf[0] = keycode; buf[1] = 0;
                    keyname = buf;
                }
            }
        #endif

        #ifdef USE_COCOA
            const char * keyname = NULL; // TODO
        #endif

        if (keyname != NULL && *keyname != 0) {
            strncpy(output, keyname, length);
            output[length - 1] = 0;
        } else {
            snprintf(output, length, "#%x", keycode);
        }
    #endif
}

void window_deinit() {
    #ifdef USE_GLFW
        glfwTerminate();
    #endif

    #ifdef USE_SDL
        SDL_DestroyWindow(hud_window->impl);
        SDL_Quit();
    #endif
}

#ifdef USE_GLFW
    #define TOOLKIT "GLFW"
#endif

#ifdef USE_SDL
    #define TOOLKIT "SDL"
#endif

#ifdef USE_GLUT
    #define TOOLKIT "GLUT"
#endif

#ifdef USE_COCOA
    #define TOOLKIT "Cocoa"
#endif

void window_title(char * suffix) {
    char title[128];

    if (suffix)
        snprintf(title, sizeof(title) - 1, "TigerSpades %s — %s (%s)", BETTERSPADES_VERSION, suffix, TOOLKIT);
    else
        sprintf(title, "TigerSpades %s (%s)", BETTERSPADES_VERSION, TOOLKIT);

    window_settitle(title);
}

void window_setmouseloc(double x, double y) {
}

void window_textinput(int allow) {
    #ifdef USE_SDL
        if (allow && !SDL_IsTextInputActive())
            SDL_StartTextInput();
        if (!allow && SDL_IsTextInputActive())
            SDL_StopTextInput();
    #endif
}

const char * window_clipboard() {
    #ifdef USE_GLFW
        return glfwGetClipboardString(hud_window->impl);
    #endif

    #ifdef USE_SDL
        return SDL_HasClipboardText() ? SDL_GetClipboardText() : NULL;
    #endif

    #ifdef USE_GLUT
        return NULL;
    #endif

    #ifdef USE_COCOA
        return NULL; // TODO
    #endif
}

void window_swapping(int value) {
    #ifdef USE_GLFW
        glfwSwapInterval(value);
    #endif

    #ifdef USE_SDL
        SDL_GL_SetSwapInterval(value);
    #endif
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
