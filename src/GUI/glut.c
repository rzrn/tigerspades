#ifdef USE_GLUT

#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

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

void window_keyboard(unsigned char key, int x, int y) {
    if (isprint(key)) text_input(hud_window, (uint8_t[]) {key, 0});

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
    reshape(hud_window, width, height);
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
        mouse_scroll(hud_window, 0, offset);
    } else
        mouse_click(hud_window, but, action, mod);
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

        mouse(hud_window, x - w / 2, y - h / 2);
    } else mouse(hud_window, x, y);
}

void window_fromsettings() {
    if (settings.fullscreen)
        glutFullScreen();
    else
        glutReshapeWindow(settings.window_width, settings.window_height);

    reshape(hud_window, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

float window_time() {
    return glutGet(GLUT_ELAPSED_TIME) / 1000.0F;
}

void window_entry(int state) {
    mouse_hover(hud_window, state == GLUT_ENTERED ? true : false);
}

void window_init(int * argc, char ** argv) {
    static WindowInstance i;
    hud_window = &i;

    glutInit(argc, argv);

    glutInitWindowSize(settings.window_width, settings.window_height);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("TigerSpades " BETTERSPADES_VERSION);

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
