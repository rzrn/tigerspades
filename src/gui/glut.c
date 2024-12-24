#ifdef USE_GLUT

#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <bs/common.h>
#include <bs/main.h>
#include <bs/window.h>
#include <bs/config.h>
#include <bs/hud.h>

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

void window_keyboard(unsigned char key, int x, int y) {
    if (isprint(key)) text_input((uint8_t[]) {key, 0});

    int mod = glutGetModifiers() & GLUT_ACTIVE_CTRL;
    window_sendkey(WINDOW_PRESS, toupper(key), mod);
}

void window_special(int key, int x, int y) {
    int mod = glutGetModifiers() & GLUT_ACTIVE_CTRL;
    window_sendkey(WINDOW_PRESS, key | GLUT_SPECIAL_MASK, mod);
}

void window_keyboard_up(unsigned char key, int x, int y) {
    int mod = glutGetModifiers() & GLUT_ACTIVE_CTRL;
    window_sendkey(WINDOW_RELEASE, toupper(key), mod);
}

void window_special_up(int key, int x, int y) {
    int mod = glutGetModifiers() & GLUT_ACTIVE_CTRL;
    window_sendkey(WINDOW_RELEASE, key | GLUT_SPECIAL_MASK, mod);
}

void window_reshape(GLint width, GLint height) {
    reshape(width, height);
}

#define GLUT_WHEEL_UP   3
#define GLUT_WHEEL_DOWN 4

void window_mouse_button(int button, int state, int x, int y) {
    int but;
    switch (button) {
        case GLUT_LEFT_BUTTON:   but = WINDOW_MOUSE_LMB; break;
        case GLUT_RIGHT_BUTTON:  but = WINDOW_MOUSE_RMB; break;
        case GLUT_MIDDLE_BUTTON: but = WINDOW_MOUSE_MMB; break;
    }

    int action;
    switch (state) {
        case GLUT_UP:   action = WINDOW_RELEASE; break;
        case GLUT_DOWN: action = WINDOW_PRESS;   break;
    }

    int mod = glutGetModifiers() & GLUT_ACTIVE_CTRL;

    if (button == GLUT_WHEEL_UP || button == GLUT_WHEEL_DOWN) {
        double offset = 0;

        offset += (button == GLUT_WHEEL_UP ? 1 : -1);
        mouse_scroll(0, offset);
    } else
        mouse_click(but, action, mod);
}

int captured = 0;

void window_mousemode(int mode) {
    if ((captured && mode == WINDOW_CURSOR_ENABLED) || (!captured && mode == WINDOW_CURSOR_DISABLED)) {
        captured = ~captured; glutSetCursor(captured ? GLUT_CURSOR_NONE : GLUT_CURSOR_INHERIT);
    }
}

int window_get_mousemode() {
    return captured ? WINDOW_CURSOR_DISABLED : WINDOW_CURSOR_ENABLED;
}

void window_settitle(char * title) {
    glutSetWindowTitle(title);
}

static double mx, my;

void window_mouseloc(double * x, double * y) {
    *x = mx;
    *y = my;
}

void window_mouse_motion(int x, int y) {
    static bool warped = false;
    mx = x; my = y;

    if (warped) {
        warped = false;
        return;
    }

    if (captured) {
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        glutWarpPointer(w / 2, h / 2);
        warped = true;

        mouse(x - w / 2, y - h / 2);
    } else mouse(x, y);
}

void window_videomode(bool windowed) {
    if (windowed)
        glutReshapeWindow(settings.window_width, settings.window_height);
    else
        glutFullScreen();
}

void window_fromsettings() {
    window_videomode(settings.windowed);

    reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

float window_time() {
    return glutGet(GLUT_ELAPSED_TIME) / 1000.0F;
}

const char * window_clipboard() {
    return NULL;
}

void window_textinput(int allow) {
}

void window_swapping(int value) {
}

void window_deinit() {
}

void window_keyname(int keycode, char * output, size_t length) {
    if (keycode == 0) { output[0] = '?'; output[1] = 0; return; }

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
            buf[0] = keycode;
            buf[1] = 0;
            keyname = buf;
        }
    }

    if (keyname != NULL && *keyname != 0)
        strnzcpy(output, keyname, length);
    else
        snprintf(output, length, "#%x", keycode);
}

void window_entry(int state) {
    mouse_hover(state == GLUT_ENTERED ? true : false);
}

void window_init(const char * title, int * argc, char ** argv) {
    glutInit(argc, argv);

    glutInitWindowSize(settings.window_width, settings.window_height);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow(title);

    glutReshapeFunc(window_reshape);
    glutKeyboardFunc(window_keyboard);
    glutKeyboardUpFunc(window_keyboard_up);
    glutSpecialFunc(window_special);
    glutSpecialUpFunc(window_special_up);
    glutMouseFunc(window_mouse_button);
    glutMotionFunc(window_mouse_motion);
    glutPassiveMotionFunc(window_mouse_motion);
    glutEntryFunc(window_entry);
}

static Idle idle     = NULL;
static Render render = NULL;

void window_display() {
    render();
    glutSwapBuffers();
}

void window_idle() {
    static double last_frame_start = 0.0F;

    double dt = window_time() - last_frame_start;
    last_frame_start = window_time();
    idle(dt);

    if (settings.vsync > 1 && (window_time() - last_frame_start) < (1.0 / settings.vsync)) {
        double sleep_s = 1.0 / settings.vsync - (window_time() - last_frame_start);
        struct timespec ts;
        ts.tv_sec = (int) sleep_s;
        ts.tv_nsec = (sleep_s - ts.tv_sec) * 1000000000.0;
        nanosleep(&ts, NULL);
    }

    fps = 1.0F / dt;

    glutPostRedisplay();
}

void window_eventloop(Idle func1, Render func2) {
    idle = func1; render = func2;

    glutIdleFunc(window_idle);
    glutDisplayFunc(window_display);
    glutMainLoop();
}

#else

typedef void dummy;

#endif
