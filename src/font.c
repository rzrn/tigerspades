/*
    Copyright © 2017–2021 ByteBit
    Copyright © 2018 vuolen
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
#include <ctype.h>
#include <math.h>

#include <hashtable.h>

#include <bs/opengl.h>
#include <bs/common.h>
#include <bs/unicode.h>
#include <bs/file.h>
#include <bs/font.h>
#include <bs/utils.h>

#define begin(T) typedef struct _##T T; struct _##T {
#define end() };
#define u8(dest)      uint8_t dest;
#define u16(dest)     uint16_t dest;
#define blob(dest, n) Blob dest;
#include <bs/bitmap.h>

#define begin(T) const size_t size##T = 0
#define end() ;
#define u8(dest)      + 1
#define u16(dest)     + 2
#define blob(dest, n) + n
#include <bs/bitmap.h>

#define begin(T) T read##T(uint8_t * buff) { T retval; size_t index = 0;
#define end() return retval; }
#define u8(dest)      retval.dest = getu8le(buff, &index);
#define u16(dest)     retval.dest = getu16le(buff, &index);
#define blob(dest, n) retval.dest = (Blob) {.data = &buff[index], .size = n}; index += n;
#include <bs/bitmap.h>

typedef struct {
    uint8_t  page, stride;
    uint16_t x, y;
} Glyph;

#ifdef USE_GL_SHORT
    #define vertex_t      GLshort
    #define VERTEX_TYPE   GL_SHORT
    #define texcoord_t    GLshort
    #define TEXCOORD_TYPE GL_SHORT
#endif

#ifdef USE_GL_FLOAT
    #define vertex_t      GLfloat
    #define VERTEX_TYPE   GL_FLOAT
    #define texcoord_t    GLfloat
    #define TEXCOORD_TYPE GL_FLOAT
#endif

#define BUFFSIZE 512
typedef struct {
    uint16_t len;
    vertex_t vertex[BUFFSIZE * 8];
    texcoord_t texcoords[BUFFSIZE * 8];
} Buffer;

typedef struct {
    uint16_t high16;
    Glyph *  table;
    float    texscale;
    size_t   npages;
    Buffer * buffers;
    GLuint   textures[64];
} Subfont;

struct _Font {
    uint32_t   replacement;
    size_t     length;
    uint8_t    height;
    Subfont *  special;
    Subfont ** subfonts;
};

static GLuint upload_page(size_t texsize, const uint8_t * buff) {
    GLuint texid;
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texsize, texsize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, buff);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texid;
}

static Subfont upload_subfont(const char * filename, size_t texsize, uint16_t height) {
    FILE * file = fopen(filename, "rb");

    if (!file) {
        log_fatal("ERROR: failed to open %s", filename);
        exit(1);
    }

    uint8_t * buff = malloc(max(sizeBitmapHeader, sizeBitmapGlyph));
    if (fread(buff, sizeBitmapHeader, 1, file) != 1) {
        log_fatal("ERROR: short font header in %s", filename);
        exit(1);
    }

    BitmapHeader header = readBitmapHeader(buff);

    if (header.height != height) {
        log_fatal("ERROR: invalid font height (given %d, expected %d)", header.height, height);
        exit(1);
    }

    Subfont font; font.high16 = header.high16;
    font.table = calloc(65536, sizeof(Glyph));

    uint8_t * pagebuff = calloc(texsize * texsize, 1);
    size_t x0 = 0, y0 = 0, pagenum = 0;

    for (;;) {
        if (fread(buff, sizeBitmapGlyph, 1, file) < 1) {
            font.textures[pagenum] = upload_page(texsize, pagebuff);
            break;
        }

        BitmapGlyph glyph = readBitmapGlyph(buff);

        size_t size = ((size_t) header.height) * ((size_t) glyph.stride);
        uint8_t * data = malloc(size);

        if (fread(data, size, 1, file) != 1) {
            log_fatal("ERROR: malformed font %s", filename);
            exit(1);
        }

        size_t width = glyph.stride * 8;

        if (x0 + width > texsize) { x0 = 0; y0 += header.height; }
        if (y0 + header.height > texsize) {
            font.textures[pagenum] = upload_page(texsize, pagebuff);
            x0 = y0 = 0; pagenum++;
        }

        font.table[glyph.low16].page   = pagenum;
        font.table[glyph.low16].stride = glyph.stride;

        font.table[glyph.low16].x = x0;
        font.table[glyph.low16].y = y0;

        for (size_t dy = 0; dy < header.height; dy++) {
            for (size_t dx = 0; dx < glyph.stride; dx++) {
                for (size_t bit = 0; bit < 8; bit++) {
                    size_t off = (y0 + dy) * texsize + (x0 + 8 * dx + 7 - bit);
                    pagebuff[off] = data[dy * glyph.stride + dx] & (1 << bit) ? 0xff : 0x00;
                }
            }
        }

        x0 += width;
    }

    font.texscale = 1.0F / ((float) texsize);
    font.npages   = pagenum + 1;
    font.buffers  = calloc(font.npages, sizeof(Buffer));

    log_info("%s (0x%04xXXXX): height = %d, npages = %d", filename, font.high16, height, font.npages);

    free(buff); free(pagebuff); fclose(file); return font;
}

Subfont unifont, uvga;

Subfont * primarySubfonts[] = {&uvga, &unifont}, * secondarySubfonts[] = {&unifont};

Font _font_primary   = {.replacement = 0xFFFD, .length = 2, .height = 16, .special = &uvga,    .subfonts = primarySubfonts};
Font _font_secondary = {.replacement = 0xFFFD, .length = 1, .height = 16, .special = &unifont, .subfonts = secondarySubfonts};

Font * const font_primary   = &_font_primary;
Font * const font_secondary = &_font_secondary;

static Font * font_selected = &_font_primary;

void font_init(void) {
    GLint max_size = 0; glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *) &max_size);

    if (max_size <= 0) {
        log_fatal("ERROR: invalid maximum texture size reported by driver: %d\n", max_size);
        exit(1);
    }

    unifont = upload_subfont("fonts/unifont.bitmap", max_size, 16);
    uvga    = upload_subfont("fonts/uvga.bitmap", max_size, 16);
}

Font * font_select(Font * font) {
    Font * font_old = font_selected;
    font_selected = font;
    return font_old;
}

Subfont * get_glyph(Font * font, uint32_t codepoint, Glyph * outglyph) {
    uint16_t high16 = codepoint >> 16;

    for (size_t k = 0; k < font_selected->length; k++) {
        Subfont * subfont = font->subfonts[k];

        if (subfont->high16 == high16) {
            Glyph glyph = subfont->table[codepoint & 0xFFFF];
            if (glyph.stride != 0) { *outglyph = glyph; return subfont; }
        }
    }

    *outglyph = font->special->table[font->replacement]; return font->special;
}

void clear_buffers(Font * font) {
    for (size_t k = 0; k < font->length; k++) {
        Subfont * subfont = font->subfonts[k];

        for (size_t i = 0; i < subfont->npages; i++)
            subfont->buffers[i].len = 0;
    }
}

static inline bool ignore(uint32_t codepoint) {
    return codepoint <= 127 && !isprint(codepoint);
}

float font_length(int scale, const char * text, int len, Codepage codepage) {
    float x = 0, length = 0;

    const char * end = len <= 0 ? (char *) UINTPTR_MAX : text + len;

    while (*text && text < end) {
        if (*text == '\n') {
            length = fmax(length, x);
            x = 0; text++;
        } else {
            size_t size = decodeSize(codepage, text[0]);
            uint32_t codepoint = decode(codepage, (const uint8_t *) text);
            text += size;

            if (ignore(codepoint)) continue;

            Glyph glyph; get_glyph(font_selected, codepoint, &glyph);
            x += scale * glyph.stride * 8;
        }
    }

    return fmax(length, x);
}

static inline void emitTexcoords(texcoord_t * buff, size_t offset, texcoord_t x, texcoord_t y, texcoord_t w, texcoord_t h) {
    texcoord_t * dest = buff + offset * 8;

    *(dest++) = x;     *(dest++) = y + h;
    *(dest++) = x + w; *(dest++) = y + h;
    *(dest++) = x + w; *(dest++) = y;
    *(dest++) = x;     *(dest++) = y;
}

static inline void emitVertex(vertex_t * buff, size_t offset, vertex_t x, vertex_t y, vertex_t w, vertex_t h) {
    vertex_t * dest = buff + offset * 8;

    *(dest++) = x;     *(dest++) = y - h;
    *(dest++) = x + w; *(dest++) = y - h;
    *(dest++) = x + w; *(dest++) = y;
    *(dest++) = x;     *(dest++) = y;
}

Vector2f font_render(float x, float y, int scale, const char * text, Codepage codepage) {
    clear_buffers(font_selected);

    float x0 = x, y0 = y, h = font_selected->height * scale;

    while (*text) {
        if (*text == '\n') {
            x0  = x;
            y0 += h;
            text++;
        } else {
            size_t size = decodeSize(codepage, text[0]);
            uint32_t codepoint = decode(codepage, (uint8_t *) text);
            text += size;

            if (ignore(codepoint)) continue;

            Glyph glyph; Subfont * subfont = get_glyph(font_selected, codepoint, &glyph);
            Buffer * buffer = &subfont->buffers[glyph.page];

            uint16_t width = glyph.stride * 8;

            emitTexcoords(buffer->texcoords, buffer->len, glyph.x, glyph.y, width, font_selected->height);
            emitVertex(buffer->vertex, buffer->len, x0, y0, scale * width, h);

            buffer->len++; x0 += scale * width;
        }
    }

    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_TEXTURE);

    for (size_t k = 0; k < font_selected->length; k++) {
        Subfont * subfont = font_selected->subfonts[k];

        glLoadIdentity(); glScalef(subfont->texscale, subfont->texscale, 1.0F);

        for (size_t i = 0; i < subfont->npages; i++) {
            Buffer * buffer = &subfont->buffers[i];
            if (buffer->len == 0) continue;

            glBindTexture(GL_TEXTURE_2D, subfont->textures[i]);
            glVertexPointer(2, VERTEX_TYPE, 0, buffer->vertex);
            glTexCoordPointer(2, TEXCOORD_TYPE, 0, buffer->texcoords);
            glDrawArrays(GL_QUADS, 0, buffer->len * 4);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);

    return (Vector2f) {x0, y0};
}

Vector2f font_centered(float x, float y, int h, const char * text, Codepage codepage)
{ return font_render(x - font_length(h, text, 0, codepage) / 2.0F, y, h, text, codepage); }
