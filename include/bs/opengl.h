/*
    Copyright © 2017–2020 ByteBit
    Copyright © 2024 rzrn

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

#ifndef OPENGL_H
#define OPENGL_H

#include <ace/types.h>

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
