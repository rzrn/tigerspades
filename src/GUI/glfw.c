#ifdef USE_GLFW

#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <BetterSpades/unicode.h>
#include <BetterSpades/common.h>
#include <BetterSpades/main.h>
#include <BetterSpades/window.h>
#include <BetterSpades/config.h>
#include <BetterSpades/hud.h>
#include <BetterSpades/font.h>
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

    if (a >= 0) mouse_click(hud_window, b, a, mods & GLFW_MOD_CONTROL);
}

static double mx, my;

static void window_impl_mouse(GLFWwindow * window, double x, double y) {
    if (!joystick_available) {
        if (glfwGetInputMode(hud_window->impl, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            mouse(hud_window, x - mx, y - my);
            mx = x;
            my = y;
        } else mouse(hud_window, x, y);
    }
}

static void window_impl_mousescroll(GLFWwindow * window, double xoffset, double yoffset) {
    mouse_scroll(hud_window, xoffset, yoffset);
}

static void window_impl_error(int i, const char * s) {
    on_error(i, s);
}

static void window_impl_reshape(GLFWwindow * window, int width, int height) {
    reshape(hud_window, width, height);
}

static void window_impl_textinput(GLFWwindow * window, unsigned int codepoint) {
    uint8_t buff[5] = {0};
    encode(UTF8, buff, codepoint);
    text_input(hud_window, buff);
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
    mouse_hover(hud_window, entered);
}

static void window_focus_callback(GLFWwindow * window, int focused) {
    mouse_focus(hud_window, focused);
}

void window_mousemode(int mode) {
    int s = glfwGetInputMode(hud_window->impl, GLFW_CURSOR);

    if (s == GLFW_CURSOR_DISABLED && mode == WINDOW_CURSOR_ENABLED)
        glfwSetInputMode(hud_window->impl, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (s == GLFW_CURSOR_NORMAL && mode == WINDOW_CURSOR_DISABLED) {
        glfwSetInputMode(hud_window->impl, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(hud_window->impl, &mx, &my);
    }
}

int window_get_mousemode() {
    int s = glfwGetInputMode(hud_window->impl, GLFW_CURSOR);
    return s == GLFW_CURSOR_DISABLED ? WINDOW_CURSOR_DISABLED : WINDOW_CURSOR_ENABLED;
}

void window_settitle(char * title) {
    glfwSetWindowTitle(hud_window->impl, title);
}

void window_mouseloc(double * x, double * y) {
    glfwGetCursorPos(hud_window->impl, x, y);
}

void window_fromsettings() {
    glfwWindowHint(GLFW_SAMPLES, settings.multisamples);
    glfwSetWindowSize(hud_window->impl, settings.window_width, settings.window_height);

    const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (settings.fullscreen)
        glfwSetWindowMonitor(hud_window->impl, glfwGetPrimaryMonitor(), 0, 0, settings.window_width,
                             settings.window_height, mode->refreshRate);
    else
        glfwSetWindowMonitor(hud_window->impl, NULL, (mode->width - settings.window_width) / 2,
                             (mode->height - settings.window_height) / 2,
                             settings.window_width, settings.window_height, 0);

    int width, height; glfwGetWindowSize(hud_window->impl, &width, &height);

    reshape(hud_window, width, height);
}

float window_time() {
    return glfwGetTime();
}

void window_init(int * argc, char ** argv) {
    static WindowInstance i;
    hud_window = &i;

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

    hud_window->impl
        = glfwCreateWindow(settings.window_width, settings.window_height, "TigerSpades " BETTERSPADES_VERSION,
                           settings.fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!hud_window->impl) {
        log_fatal("Could not open window");
        glfwTerminate();
        exit(1);
    }

    const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(hud_window->impl, (mode->width - settings.window_width) / 2.0F,
                     (mode->height - settings.window_height) / 2.0F);
    glfwShowWindow(hud_window->impl);

    glfwMakeContextCurrent(hud_window->impl);

    glfwSetFramebufferSizeCallback(hud_window->impl, window_impl_reshape);
    glfwSetCursorPosCallback(hud_window->impl, window_impl_mouse);
    glfwSetKeyCallback(hud_window->impl, window_impl_keys);
    glfwSetMouseButtonCallback(hud_window->impl, window_impl_mouseclick);
    glfwSetScrollCallback(hud_window->impl, window_impl_mousescroll);
    glfwSetCharCallback(hud_window->impl, window_impl_textinput);
    glfwSetWindowFocusCallback(hud_window->impl, window_focus_callback);
    glfwSetCursorEnterCallback(hud_window->impl, window_cursor_enter_callback);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(hud_window->impl, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

static void gamepad_translate_key(GLFWgamepadstate * state, GLFWgamepadstate * old, int gamepad, WindowKey key) {
    if (!old->buttons[gamepad] && state->buttons[gamepad]) {
        keys(hud_window, key, WINDOW_PRESS, 0);

        if (hud_active->input_keyboard)
            hud_active->input_keyboard(key, WINDOW_PRESS, 0, 0);
    } else if (old->buttons[gamepad] && !state->buttons[gamepad]) {
        keys(hud_window, key, WINDOW_RELEASE, 0);

        if (hud_active->input_keyboard)
            hud_active->input_keyboard(key, WINDOW_RELEASE, 0, 0);
    }
}

static void gamepad_translate_button(GLFWgamepadstate * state, GLFWgamepadstate * old, int gamepad, WindowButton button) {
    if (!old->buttons[gamepad] && state->buttons[gamepad]) {
        mouse_click(hud_window, button, WINDOW_PRESS, 0);
    } else if (old->buttons[gamepad] && !state->buttons[gamepad]) {
        mouse_click(hud_window, button, WINDOW_RELEASE, 0);
    }
}

void window_update() {
    glfwSwapBuffers(hud_window->impl);
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

            if (glfwGetInputMode(hud_window->impl, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
                mouse(hud_window, dx, dy);
            else
                mouse(hud_window, joystick_mouse[0], joystick_mouse[1]);

            gamepad_translate_button(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_A, WINDOW_MOUSE_LMB);
            gamepad_translate_button(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_B, WINDOW_MOUSE_RMB);
        }

        joystick_state = state;
    }
}

void window_eventloop(Idle idle, Render render) {
    double last_frame_start = 0.0F;

    while (!glfwWindowShouldClose(hud_window->impl)) {
        double dt = window_time() - last_frame_start;
        last_frame_start = window_time();

        idle(dt);
        render();
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
}

#else

typedef void dummy;

#endif
