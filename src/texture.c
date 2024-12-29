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

#include <math.h>

#include <bs/texture.h>
#include <bs/opengl.h>
#include <bs/common.h>
#include <bs/file.h>
#include <bs/map.h>

#include <log.h>
#include <lodepng/lodepng.c>

struct _Texture {
    unsigned int width, height;
    GLuint texture_id;
    unsigned char * pixels;
};

#define TEXTURE_TOTAL (TEXTURE_LAST + 1)
Texture _textures[TEXTURE_TOTAL];

Texture _texture_color_selection, _texture_minimap, _texture_dummy, _texture_gradient;

Texture * const texture_color_selection = &_texture_color_selection;
Texture * const texture_minimap         = &_texture_minimap;
Texture * const texture_dummy           = &_texture_dummy;
Texture * const texture_gradient        = &_texture_gradient;

Texture * texture(enum Texture index) { return &_textures[index]; }

const char * texture_filename(enum Texture index) {
    switch (index) {
        case TEXTURE_SPLASH:       return "png/splash.png";

        case TEXTURE_ZOOM_SEMI:    return "png/semi.png";
        case TEXTURE_ZOOM_SMG:     return "png/smg.png";
        case TEXTURE_ZOOM_SHOTGUN: return "png/shotgun.png";

        case TEXTURE_WHITE:        return "png/white.png";
        case TEXTURE_CROSSHAIR1:   return "png/crosshair1.png";
        case TEXTURE_CROSSHAIR2:   return "png/crosshair2.png";
        case TEXTURE_INDICATOR:    return "png/indicator.png";

        case TEXTURE_PLAYER:       return "png/player.png";
        case TEXTURE_MEDICAL:      return "png/medical.png";
        case TEXTURE_INTEL:        return "png/intel.png";
        case TEXTURE_COMMAND:      return "png/command.png";
        case TEXTURE_TRACER:       return "png/tracer.png";

#ifdef USE_TOUCH
        case TEXTURE_UI_KNOB:      return "png/ui/knob.png";
        case TEXTURE_UI_JOYSTICK:  return "png/ui/joystick.png";
#endif

        case TEXTURE_UI_RELOAD:    return "png/ui/reload.png";
        case TEXTURE_UI_BG:        return "png/ui/bg.png";
        case TEXTURE_UI_INPUT:     return "png/ui/input.png";
        case TEXTURE_UI_COLLAPSED: return "png/ui/collapsed.png";
        case TEXTURE_UI_EXPANDED:  return "png/ui/expanded.png";
        case TEXTURE_UI_FLAGS:     return "png/ui/flags.png";
        case TEXTURE_UI_ALERT:     return "png/ui/alert.png";
    }

    return NULL;
}

static char * texture_flags[] = {
    "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AN", "AO", "AR", "AS", "AT", "AU", "AW", "AX", "AZ",
    "BA", "BB", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BM", "BN", "BO", "BR", "BS", "BT", "BV",
    "BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG", "CH", "CI", "CK", "CL", "CM", "CN", "CO", "CR",
    "CS", "CU", "CV", "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO", "DZ", "EC", "EE", "EG", "EH",
    "ER", "ES", "ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB", "GD", "GE", "GF", "GH", "GI",
    "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", "HM", "HN", "HR", "HT",
    "HU", "ID", "IE", "IL", "IN", "IO", "IQ", "IR", "IS", "IT", "JM", "JO", "JP", "KE", "KG", "KH",
    "KI", "KM", "KN", "KP", "KR", "KW", "KY", "KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT",
    "LU", "LV", "LY", "MA", "MC", "MD", "ME", "MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP", "MQ",
    "MR", "MS", "MT", "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", "NF", "NG", "NI", "NL",
    "NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN", "PR",
    "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU", "RW", "SA", "SB", "SC", "SD", "SE", "SG",
    "SH", "SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "ST", "SV", "SY", "SZ", "TC", "TD", "TF",
    "TG", "TH", "TJ", "TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", "UM",
    "US", "UY", "UZ", "VA", "VC", "VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YT", "ZA", "ZM",
    "ZW"
};

static int texture_flag_cmp(const void * a, const void * b) {
    return strcmp(a, *(const void * const *) b);
}

int texture_flag_index(const char * country) {
    char ** res = bsearch(country, texture_flags, sizeof(texture_flags) / sizeof(texture_flags[0]), sizeof(char*), texture_flag_cmp);
    return res ? (res - texture_flags) : -1;
}

void texture_flag_offset(int index, float * u, float * v) {
    if (index >= 0) {
        *u = (index % 16) * (16.0F / 256.0F);
        *v = (index / 16) * (16.0F / 256.0F);
    } else {
        *u = 0.0625F;
        *v = 0.9375F;
    }
}

static inline GLuint GLfiltering(Filtering filter) {
    switch (filter) {
        case TEXTURE_FILTER_NEAREST: return GL_NEAREST;
        case TEXTURE_FILTER_LINEAR:  return GL_LINEAR;
        default:                     return GL_NEAREST;
    }
}

void texture_filter(Texture * t, Filtering filter) {
    glBindTexture(GL_TEXTURE_2D, t->texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLfiltering(filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLfiltering(filter));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void texture_create_buffer(Texture * t, const char * name, unsigned int width, unsigned int height, unsigned char * buff, int new) {
    if (new) glGenTextures(1, &t->texture_id);
    t->width  = width;
    t->height = height;
    t->pixels = buff;

    texture_resize_pow2(t, name, max(width, height));

    glBindTexture(GL_TEXTURE_2D, t->texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void texture_delete(Texture * t) {
    if (t->pixels) free(t->pixels);
    glDeleteTextures(1, &t->texture_id);
}

static inline void draw_rectangle(float * vertices, float * texcoords) {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

static inline void texture_draw_rectangle(float x, float y, float w, float h, float u1, float u2, float v1, float v2) {
    float x1 = x, x2 = x + w, y1 = y, y2 = y - h;
    float vertices[12] = {x1, y1, x1, y2, x2, y2, x1, y1, x2, y2, x2, y1};

    float texcoords[12] = {u1, v1, u1, v2, u2, v2, u1, v1, u2, v2, u2, v1};

    draw_rectangle(vertices, texcoords);
}

#define texture_emit_rotated(tx, ty, x, y, a) cos(a) * (x) - sin(a) * (y) + (tx), sin(a) * (x) + cos(a) * (y) + (ty)

static inline void texture_draw_rotated_rectangle(float x, float y, float w, float h, float phi, float u1, float u2, float v1, float v2) {
    float vertices[12] = {
        texture_emit_rotated(x, y, -w / 2, +h / 2, phi), texture_emit_rotated(x, y, -w / 2, -h / 2, phi),
        texture_emit_rotated(x, y, +w / 2, -h / 2, phi), texture_emit_rotated(x, y, -w / 2, +h / 2, phi),
        texture_emit_rotated(x, y, +w / 2, -h / 2, phi), texture_emit_rotated(x, y, +w / 2, +h / 2, phi)
    };

    float texcoords[12] = {u1, v1, u1, v2, u2, v2, u1, v1, u2, v2, u2, v1};

    draw_rectangle(vertices, texcoords);
}

void texture_draw_sector(Texture * t, float x, float y, float w, float h, float u, float v, float us, float vs) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, t->texture_id);

    float du = 0.5f / t->width, dv = 0.5f / t->height;
    texture_draw_rectangle(x, y, w, h, u + du, u + us - du, v + dv, v + vs - dv);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void texture_draw(Texture * t, float x, float y, float w, float h) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, t->texture_id);

    texture_draw_rectangle(x, y, w, h, 0.0f, 1.0f, 0.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void texture_draw_rotated(Texture * t, float x, float y, float w, float h, float phi) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, t->texture_id);

    texture_draw_rotated_rectangle(x, y, w, h, phi, 0.0f, 1.0f, 0.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void texture_draw_empty(float x, float y, float w, float h) {
    texture_draw_rectangle(x, y, w, h, 0.0f, 1.0f, 0.0f, 1.0f);
}

void texture_draw_empty_rotated(float x, float y, float w, float h, float phi) {
    texture_draw_rotated_rectangle(x, y, w, h, phi, 0.0f, 1.0f, 0.0f, 1.0f);
}

void texture_load(enum Texture index, Filtering filter) {
    Texture * t = &_textures[index];
    const char * filename = texture_filename(index);

    int sz = file_size(filename);
    void * data = file_load(filename);
    int error = lodepng_decode32(&t->pixels, &t->width, &t->height, data, sz);
    free(data);

    if (error) {
        log_warn("%s: could not load texture (%u): %s", filename, error, lodepng_error_text(error));
        return;
    }

    texture_resize_pow2(t, filename, 0);

    glGenTextures(1, &t->texture_id);
    glBindTexture(GL_TEXTURE_2D, t->texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLfiltering(filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLfiltering(filter));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static inline bool has_npot_textures() {
    return strstr((const char *) glGetString(GL_EXTENSIONS), "ARB_texture_non_power_of_two") != NULL;
}

#define BYTE0(col) ((col) & 0xFF)
#define BYTE1(col) (((col) >> 8) & 0xFF)
#define BYTE2(col) (((col) >> 16) & 0xFF)
#define BYTE3(col) (((col) >> 24) & 0xFF)

void texture_resize_pow2(Texture * t, const char * name, unsigned int min_size) {
    if (!t->pixels) return;

    unsigned int max_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *) &max_size);
    max_size = max(max_size, min_size);

    unsigned int w = 1, h = 1;
    if (has_npot_textures()) {
        if (t->width <= max_size && t->height <= max_size)
            return;
        w = t->width;
        h = t->height;
    } else {
        while (w < t->width)
            w += w;
        while (h < t->height)
            h += h;
    }

    w = min(w, max_size);
    h = min(h, max_size);

    if (t->width == w && t->height == h)
        return;

    log_info("%s: texture original: %i:%i now: %i:%i limit: %i", name, t->width, t->height, w, h, max_size);

    if (!t->pixels)
        return;

    unsigned int * pixels_new = malloc(w * h * sizeof(unsigned int));
    CHECK_ALLOCATION_ERROR(pixels_new)
    for (unsigned int y = 0; y < h; y++) {
        for (unsigned int x = 0; x < w; x++) {
            float px = (float) x / (float) w * (float) t->width;
            float py = (float) y / (float) h * (float) t->height;
            float u = px - (int) px;
            float v = py - (int) py;

            unsigned int aa = ((unsigned int *) t->pixels)[(int) px + (int) py * t->width];
            unsigned int ba = ((unsigned int *) t->pixels)[min((int) px + (int) py * t->width + 1, t->width * t->height)];
            unsigned int ab = ((unsigned int *) t->pixels)[min((int) px + (int) py * t->width + t->width, t->width * t->height)];
            unsigned int bb = ((unsigned int *) t->pixels)[min((int) px + (int) py * t->width + 1 + t->width, t->width * t->height)];

            pixels_new[x + y * w] = 0;
            pixels_new[x + y * w] |= (int) ((1.0F - v) * u * BYTE0(ba) + (1.0F - v) * (1.0F - u) * BYTE0(aa)
                                            + v * u * BYTE0(bb) + v * (1.0F - u) * BYTE0(ab));
            pixels_new[x + y * w] |= (int) ((1.0F - v) * u * BYTE1(ba) + (1.0F - v) * (1.0F - u) * BYTE1(aa)
                                            + v * u * BYTE1(bb) + v * (1.0F - u) * BYTE1(ab))
                << 8;
            pixels_new[x + y * w] |= (int) ((1.0F - v) * u * BYTE2(ba) + (1.0F - v) * (1.0F - u) * BYTE2(aa)
                                            + v * u * BYTE2(bb) + v * (1.0F - u) * BYTE2(ab))
                << 16;
            pixels_new[x + y * w] |= (int) ((1.0F - v) * u * BYTE3(ba) + (1.0F - v) * (1.0F - u) * BYTE3(aa)
                                            + v * u * BYTE3(bb) + v * (1.0F - u) * BYTE3(ab))
                << 24;
        }
    }

    t->width = w;
    t->height = h;
    free(t->pixels);
    t->pixels = (unsigned char *) pixels_new;
}

RGB3i texture_block_color(int x, int y) {
    int base[3][8] = {{15, 31, 31, 31, 0, 0, 0, 31}, {15, 0, 15, 31, 31, 31, 0, 0}, {15, 0, 0, 0, 0, 31, 31, 31}};

    uint32_t r = base[0][y] + x * (base[0][y] * 2 + 2) * (base[0][y] > 0);
    uint32_t g = base[1][y] + x * (base[1][y] * 2 + 2) * (base[1][y] > 0);
    uint32_t b = base[2][y] + x * (base[2][y] * 2 + 2) * (base[2][y] > 0);

    if (x >= 4) {
        r += ((x - 3) * 64 - 33) * (!base[0][y]);
        g += ((x - 3) * 64 - 33) * (!base[1][y]);
        b += ((x - 3) * 64 - 33) * (!base[2][y]);
    }

    return (RGB3i) {min(r, 255), min(g, 255), min(b, 255)};
}

void texture_gradient_fog(unsigned int * gradient) {
    int size = 512;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int d = min(hypot2f(x - size / 2, y - size / 2) / (float) size * 2.0F * 255.0F, 255);
            writeRGBA(gradient + x + y * size, (TrueColor) {d, d, d, 255});
        }
    }
}

void texture_minimap_clear() {
    uint32_t * buff = malloc(map_size_x * map_size_z * sizeof(uint32_t));

    for (int x = 0; x < map_size_x; x++) {
        for (int z = 0; z < map_size_z; z++) {
            uint32_t * out = buff + x + z * map_size_z;
            writeRGBA(out, ISGRID(x, z) ? White : Sky);
        }
    }

    texture_subimage(texture_minimap, 0, 0, map_size_x, map_size_z, buff);
    free(buff);
}

void texture_init() {
    for (enum Texture i = TEXTURE_FIRST; i <= TEXTURE_LAST; i++)
        texture_load(i, TEXTURE_FILTER_NEAREST);

    unsigned int pixels[64 * 64];
    memset(pixels, 0, sizeof(pixels));

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            RGB3i color = texture_block_color(x, y);

            for (int ys = 0; ys < 6; ys++)
                for (int xs = 0; xs < 6; xs++)
                    writeRGBA(pixels + (x * 8 + xs) + (y * 8 + ys) * 64, opaque(color));
        }
    }

    texture_create_buffer(texture_color_selection, "_texture_color_selection", 64, 64, (unsigned char *) pixels, 1);

    texture_create_buffer(texture_minimap, "_texture_minimap", map_size_x, map_size_z, NULL, 1);

    unsigned int * gradient = malloc(512 * 512 * sizeof(unsigned int));
    CHECK_ALLOCATION_ERROR(gradient)
    texture_gradient_fog(gradient);
    texture_create_buffer(texture_gradient, "_texture_gradient", 512, 512, (unsigned char *) gradient, 1);
    texture_filter(texture_gradient, TEXTURE_FILTER_LINEAR);

    texture_create_buffer(texture_dummy, "_texture_dummy", 1, 1, (unsigned char[]) {0, 0, 0, 0}, 1);
}

void texture_bind(Texture * t) {
    glBindTexture(GL_TEXTURE_2D, t->texture_id);
}

float texture_width(Texture * t) {
    return t->width;
}

float texture_height(Texture * t) {
    return t->height;
}

Texture * texture_alloc() {
    return calloc(1, sizeof(Texture));
}

void texture_subimage(Texture * t, int xoffset, int yoffset, size_t width, size_t height, const void * buff) {
    glBindTexture(GL_TEXTURE_2D, t->texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buff);
    glBindTexture(GL_TEXTURE_2D, 0);
}
