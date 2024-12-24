#ifndef GUI_H
#define GUI_H

#ifdef USE_GLFW
    #include <GLFW/glfw3.h>
#endif

#ifdef USE_SDL
    #define SDL_MAIN_HANDLED
    #include <SDL2/SDL.h>
#endif

#ifdef USE_GLUT
    #ifdef __APPLE__
        #include <GLUT/glut.h>
    #else
        #include <GL/glut.h>
    #endif
#endif

#endif
