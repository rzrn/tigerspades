#ifndef OPENGL_H
#define OPENGL_H

#include <AceOfSpades/types.h>

#ifndef OPENGL_ES
    #define GLEW_STATIC
    #include <GL/glew.h>
#else
    #ifdef USE_SDL
        #include <SDL2/SDL_opengles.h>
    #endif

    #define glColor3f(r, g, b)  glColor4f(r, g, b, 1.0F)
    #define glColor3ub(r, g, b) glColor4ub(r, g, b, 255)
    #define glDepthRange(a, b)  glDepthRangef(a, b)
    #define glClearDepth(a)     glClearDepthf(a)
#endif

static inline void glColorRGB3i(const RGB3i color)
{ glColor3ub(color.r, color.g, color.b); }

static inline void glColorRGB3ib(const RGB3i color, const float brightness)
{ glColor3ub(color.r * brightness, color.g * brightness, color.b * brightness); }

#endif
