/*
    Copyright © 2017–2020, 2022 ByteBit
    Copyright © 2019 teodor6140
    Copyright © 2022 Fran6nd
    Copyright © 2022 Julius C. Enriquez
    Copyright © 2021, 2023–2026 rzrn

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifdef USE_GLFW

#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <bs/unicode.h>
#include <bs/common.h>
#include <bs/main.h>
#include <bs/window.h>
#include <bs/config.h>
#include <bs/hud.h>
#include <bs/font.h>

#include <bs/gui/toolkit.h>

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

// See: https://github.com/glfw/glfw/blob/master/tests/events.c
// Copyright © 2010–2024 Camilla Löwy <elmindreda@glfw.org>
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

static bool joystick_available = false;
static int joystick_id;
static float joystick_mouse[2] = {0, 0};
static GLFWgamepadstate joystick_state;

static void window_impl_joystick(int jid, int event) {
    if (event == GLFW_CONNECTED) {
        joystick_available = true;
        joystick_id = jid;
        log_info("Joystick detected: %s", glfwGetJoystickName(joystick_id));
    } else if (event == GLFW_DISCONNECTED) {
        joystick_available = false;
        log_info("Joystick removed: %s", glfwGetJoystickName(joystick_id));
    }
}

static void window_impl_mouseclick(GLFWwindow * window, int button, int action, int mods) {
    int b = 0;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:   b = WINDOW_MOUSE_LMB; break;
        case GLFW_MOUSE_BUTTON_RIGHT:  b = WINDOW_MOUSE_RMB; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: b = WINDOW_MOUSE_MMB; break;
    }

    int a = -1;
    switch (action) {
        case GLFW_RELEASE: a = WINDOW_RELEASE; break;
        case GLFW_PRESS: a = WINDOW_PRESS; break;
    }

    if (a >= 0) mouse_click(b, a, mods & GLFW_MOD_CONTROL);
}

static double mx, my;

static void window_impl_mouse(GLFWwindow * window, double x, double y) {
    if (!joystick_available) {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            mouse(x - mx, y - my);
            mx = x;
            my = y;
        } else mouse(x, y);
    }
}

static void window_impl_mousescroll(GLFWwindow * window, double xoffset, double yoffset) {
    mouse_scroll(xoffset, yoffset);
}

static void window_impl_error(int i, const char * s) {
    on_error(i, s);
}

static void window_impl_reshape(GLFWwindow * window, int width, int height) {
    reshape(width, height);
}

static void window_impl_textinput(GLFWwindow * window, unsigned int codepoint) {
    uint8_t buff[5] = {0};
    encode(UTF8, buff, codepoint);
    text_input(buff);
}

static void window_impl_keys(GLFWwindow * window, int key, int scancode, int action, int mods) {
    int a = -1;
    switch (action) {
        case GLFW_RELEASE: a = WINDOW_RELEASE; break;
        case GLFW_PRESS:   a = WINDOW_PRESS;   break;
        case GLFW_REPEAT:  a = WINDOW_REPEAT;  break;
    }

    window_sendkey(a, key, mods & GLFW_MOD_CONTROL);
}

static void window_cursor_enter_callback(GLFWwindow * window, int entered) {
    mouse_hover(entered);
}

static void window_focus_callback(GLFWwindow * window, int focused) {
    mouse_focus(focused);
}

static GLFWwindow * window = NULL;

void window_mousemode(int mode) {
    int s = glfwGetInputMode(window, GLFW_CURSOR);

    if (s == GLFW_CURSOR_DISABLED && mode == WINDOW_CURSOR_ENABLED)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (s == GLFW_CURSOR_NORMAL && mode == WINDOW_CURSOR_DISABLED) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(window, &mx, &my);
    }
}

int window_get_mousemode(void) {
    int s = glfwGetInputMode(window, GLFW_CURSOR);
    return s == GLFW_CURSOR_DISABLED ? WINDOW_CURSOR_DISABLED : WINDOW_CURSOR_ENABLED;
}

void window_settitle(char * title) {
    glfwSetWindowTitle(window, title);
}

void window_mouseloc(double * x, double * y) {
    glfwGetCursorPos(window, x, y);
}

void window_videomode(bool windowed) {
    const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    if (windowed)
        glfwSetWindowMonitor(window, NULL,
                             (mode->width - settings.window_width) / 2,
                             (mode->height - settings.window_height) / 2,
                             settings.window_width, settings.window_height, 0);
    else
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0,
                             settings.window_width, settings.window_height, mode->refreshRate);
}

void window_fromsettings(void) {
    glfwWindowHint(GLFW_SAMPLES, settings.multisamples);
    glfwSetWindowSize(window, settings.window_width, settings.window_height);

    window_videomode(settings.windowed);

    int width, height; glfwGetWindowSize(window, &width, &height);
    reshape(width, height);
}

float window_time(void) {
    return glfwGetTime();
}

const char * window_clipboard(void) {
    return glfwGetClipboardString(window);
}

void window_textinput(int allow) {
}

void window_swapping(int value) {
    glfwSwapInterval(value);
}

void window_deinit(void) {
    glfwTerminate();
}

void window_keyname(int keycode, char * output, size_t length) {
    if (keycode == 0) { output[0] = '?'; output[1] = 0; return; }

    const char * keyname = glfw_get_fnkey_name(keycode);
    if (keyname == NULL) keyname = glfwGetKeyName(keycode, 0);

    if (keyname != NULL && *keyname != 0)
        strnzcpy(output, keyname, length);
    else
        snprintf(output, length, "#%x", keycode);
}

void window_init(const char * title, int * argc, char ** argv) {
    glfwWindowHint(GLFW_VISIBLE, 0);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    #ifdef OPENGL_ES
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    #endif

    glfwSetErrorCallback(window_impl_error);

    if (!glfwInit()) {
        log_fatal("GLFW3 init failed");
        exit(1);
    }

    glfwSetJoystickCallback(window_impl_joystick);

    if (settings.multisamples > 0) {
        glfwWindowHint(GLFW_SAMPLES, settings.multisamples);
    }

    /*
    #FIXME: This is intended to fix the issue #145.
    This is dirty because it disables the application-level Hi-DPI support for every installation
    instead of being applied only to those who needs it.
    */
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

    window = glfwCreateWindow(settings.window_width, settings.window_height, title,
                              settings.windowed ? NULL : glfwGetPrimaryMonitor(), NULL);

    if (window == NULL) {
        log_fatal("Could not open window");
        glfwTerminate();
        exit(1);
    }

    const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - settings.window_width) / 2.0F,
                     (mode->height - settings.window_height) / 2.0F);
    glfwShowWindow(window);

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, window_impl_reshape);
    glfwSetCursorPosCallback(window, window_impl_mouse);
    glfwSetKeyCallback(window, window_impl_keys);
    glfwSetMouseButtonCallback(window, window_impl_mouseclick);
    glfwSetScrollCallback(window, window_impl_mousescroll);
    glfwSetCharCallback(window, window_impl_textinput);
    glfwSetWindowFocusCallback(window, window_focus_callback);
    glfwSetCursorEnterCallback(window, window_cursor_enter_callback);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

static void gamepad_translate_key(GLFWgamepadstate * state, GLFWgamepadstate * old, int gamepad, WindowKey key) {
    if (!old->buttons[gamepad] && state->buttons[gamepad]) {
        keys(key, WINDOW_PRESS, 0);

        if (hud_active->input_keyboard)
            hud_active->input_keyboard(key, WINDOW_PRESS, 0, 0);
    } else if (old->buttons[gamepad] && !state->buttons[gamepad]) {
        keys(key, WINDOW_RELEASE, 0);

        if (hud_active->input_keyboard)
            hud_active->input_keyboard(key, WINDOW_RELEASE, 0, 0);
    }
}

static void gamepad_translate_button(GLFWgamepadstate * state, GLFWgamepadstate * old, int gamepad, WindowButton button) {
    if (!old->buttons[gamepad] && state->buttons[gamepad]) {
        mouse_click(button, WINDOW_PRESS, 0);
    } else if (old->buttons[gamepad] && !state->buttons[gamepad]) {
        mouse_click(button, WINDOW_RELEASE, 0);
    }
}

void window_update(void) {
    glfwSwapBuffers(window);
    glfwPollEvents();

    if (joystick_available && glfwJoystickIsGamepad(joystick_id)) {
        GLFWgamepadstate state;

        if (glfwGetGamepadState(joystick_id, &state)) {
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_UP, WINDOW_KEY_TOOL1);
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, WINDOW_KEY_TOOL3);
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, WINDOW_KEY_TOOL4);
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, WINDOW_KEY_TOOL2);

            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_START, WINDOW_KEY_ESCAPE);
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, WINDOW_KEY_SPACE);
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_LEFT_THUMB, WINDOW_KEY_CROUCH);
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, WINDOW_KEY_SPRINT);
            gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_X, WINDOW_KEY_RELOAD);

            window_pressed_keys[WINDOW_KEY_UP]    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -0.25F;
            window_pressed_keys[WINDOW_KEY_DOWN]  = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.25F;
            window_pressed_keys[WINDOW_KEY_LEFT]  = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -0.25F;
            window_pressed_keys[WINDOW_KEY_RIGHT] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.25F;

            double dx = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * 15.0F;
            double dy = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * 15.0F;

            joystick_mouse[0] += dx;
            joystick_mouse[1] += dy;

            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
                mouse(dx, dy);
            else
                mouse(joystick_mouse[0], joystick_mouse[1]);

            gamepad_translate_button(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_A, WINDOW_MOUSE_LMB);
            gamepad_translate_button(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_B, WINDOW_MOUSE_RMB);
        }

        joystick_state = state;
    }
}

int main(int argc, char * argv[]) {
    int errval = game_main(argc, argv);
    if (errval != 0) return errval;

    double last_frame_start = 0.0F;

    while (!glfwWindowShouldClose(window)) {
        double dt = window_time() - last_frame_start;
        last_frame_start = window_time();

        game_idle(dt);
        game_display();
        window_update();

        if (settings.vsync > 1 && (window_time() - last_frame_start) < (1.0 / settings.vsync)) {
            double sleep_s = 1.0 / settings.vsync - (window_time() - last_frame_start);
            struct timespec ts;
            ts.tv_sec = (int) sleep_s;
            ts.tv_nsec = (sleep_s - ts.tv_sec) * 1000000000.0;
            nanosleep(&ts, NULL);
        }

        fps = 1.0F / dt;
    }

    return 0;
}

#else

typedef void dummy;

#endif
