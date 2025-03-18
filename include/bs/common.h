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

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <math.h>

#include <ace/types.h>

#define UNUSED(x) ((void) x)

#define _TOSTRING(x) #x
#define TOSTRING(x) _TOSTRING(x)

#ifdef _WIN32
    #define OS_WINDOWS
#endif

#ifdef __linux__
    #define OS_LINUX
#endif

#ifdef __APPLE__
    #define OS_APPLE
#endif

#ifdef __HAIKU__
    #define OS_HAIKU
#endif

#if defined(OS_WINDOWS)
    #define OS "Windows"
#elif defined(OS_LINUX)
    #define OS "Linux"
#elif defined(OS_APPLE)
    #define OS "Mac"
#elif defined(OS_HAIKU)
    #define OS "Haiku"
#elif defined(USE_TOUCH)
    #define OS "Android"
#else
    #define OS "Unknown"
#endif

#if defined(__alpha__)
    #define ARCH "Alpha"
#elif defined(__x86_64__)
    #define ARCH "x86-64"
#elif defined(__arm__)
    #define ARCH "ARM"
#elif defined(__aarch64__)
    #define ARCH "ARM64"
#elif defined(__i386__)
    #define ARCH "i386"
#elif defined(__ia64__)
    #define ARCH "IA-64"
#elif defined(__m68k__)
    #define ARCH "m68k"
#elif defined(__mips__)
    #define ARCH "MIPS"
#elif defined(_ARCH_PPC64) || defined(__ppc64__) || defined(__PPC64__) || defined(__powerpc64__) || defined(__ppc_64)
    #define ARCH "PPC64"
#elif defined(_ARCH_PPC) || defined(__ppc) || defined(__ppc__) || defined(__PPC__) || defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__)
    #define ARCH "PowerPC"
#elif defined(__riscv)
    #define ARCH "RISC-V"
#elif defined(__sparc__)
    #define ARCH "SPARC"
#else
    #define ARCH ""
#endif

#define BSMAJOR 0
#define BSMINOR 1
#define BSPATCH 7

#define BSVERSION "v" TOSTRING(BSMAJOR) "." TOSTRING(BSMINOR) "." TOSTRING(BSPATCH)
#define BSSUMMARY BSVERSION " " ARCH " " GIT_COMMIT_HASH

#ifdef USE_RPC
    #include <discord_rpc.h>
#endif

#ifndef min
    #define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
    #define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define clamp(m, M, x) (min(M, max(m, x)))

#define absf(a) (((a) > 0) ? (a) : -(a))

#define lengthof(x) (sizeof(x) / sizeof(x[0]))

static inline float sqrf(float x)    { return x * x; }
static inline float cubef(float x)   { return x * x * x; }
static inline float fourthf(float x) { return x * x * x * x; }

static inline float norm2f(float x1, float y1, float x2, float y2)
{ return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1); }

static inline float norm3f(float x1, float y1, float z1, float x2, float y2, float z2)
{ return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1); }

static inline float normv3f(const Vector3f v1, const Vector3f v2)
{ return norm3f(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z); }

static inline int norm3i(int x1, int y1, int z1, int x2, int y2, int z2)
{ return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1); }

// Vectors should be normalized.
static inline float angle3f(float x1, float y1, float z1, float x2, float y2, float z2)
{ return acosf(x1 * x2 + y1 * y2 + z1 * z2); }

static inline float hypot2f(float x, float y) { return sqrtf(x * x + y * y); }
static inline float hypot3f(float x, float y, float z) { return sqrtf(x * x + y * y + z * z); }

#define PI      3.14159265F
#define TAU     (PI * 2.0F)
#define HALFPI  (PI * 0.5F)
#define EPSILON 0.005F

#define MOUSE_SENSITIVITY 0.002F

typedef enum {
    CHAT_NO_INPUT   = 0,
    CHAT_ALL_INPUT  = 1,
    CHAT_TEAM_INPUT = 2,
} ChatInputMode;

typedef enum {
    VER075, VER076, VER07X
} GameVersion;

typedef enum {
    UTF8, ASCII, CP437, CP1252
} Codepage;

extern const RGBA4i White, Black, Red, Green, Blue, Yellow, Cyan, Magenta, Sky;

extern const RGB3i Gray;

extern ChatInputMode chat_input_mode;
extern float last_cy;

extern int fps;

extern char chat[2][10][256];
extern RGBA4i chat_color[2][10];
extern float chat_timer[2][10];
extern char chat_popup[256];
extern float chat_popup_timer;
extern float chat_popup_duration;
extern RGBA4i chat_popup_color;
void chat_add(int channel, RGBA4i, const char *, size_t, Codepage);
void chat_showpopup(const char *, size_t, Codepage, float duration, RGBA4i);
const char * reason_disconnect(int code);

int ms_rand(void);

#include <stdlib.h>
#include <log.h>

#define CHECK_ALLOCATION_ERROR(ret)                                                        \
    if (!ret) {                                                                            \
        log_fatal("Critical error: memory allocation failed (%s:%d)", __func__, __LINE__); \
        exit(1);                                                                           \
    }

static inline uint8_t decode8le(uint8_t * const buff)
{ return buff[0]; }

static inline uint16_t decode16le(uint8_t * const buff)
{ return (uint16_t) buff[0] << 0
       | (uint16_t) buff[1] << 8; }

static inline uint32_t decode32le(uint8_t * const buff)
{ return (uint32_t) buff[0] << 0
       | (uint32_t) buff[1] << 8
       | (uint32_t) buff[2] << 16
       | (uint32_t) buff[3] << 24; }

static inline void encode8le(uint8_t * const buff, uint8_t value)
{ buff[0] = value; }

static inline void encode16le(uint8_t * const buff, uint16_t value)
{ buff[0] = (value >> 0) & 0xFF;
  buff[1] = (value >> 8) & 0xFF; }

static inline void encode32le(uint8_t * const buff, uint32_t value)
{ buff[0] = (value >> 0)  & 0xFF;
  buff[1] = (value >> 8)  & 0xFF;
  buff[2] = (value >> 16) & 0xFF;
  buff[3] = (value >> 24) & 0xFF; }

#define DEFGETTER(T, U, ident, decoder) static inline T ident(uint8_t * const buff, size_t * index) \
                                        { union _Blob { T val; U data; } ret; \
                                          ret.data = decoder(buff + *index); \
                                          *index += sizeof(U); return ret.val; }

#define DEFSETTER(T, U, ident, encoder) static inline void ident(uint8_t * buff, size_t * index, T value) \
                                        { union _Blob { T val; U data; } ret; \
                                          ret.val = value; encoder(buff + *index, ret.data); \
                                          *index += sizeof(U); }

DEFGETTER(uint8_t,  uint8_t,  getu8le,   decode8le)
DEFGETTER(uint16_t, uint16_t, getu16le,  decode16le)
DEFGETTER(uint32_t, uint32_t, getu32le,  decode32le)
DEFGETTER(int8_t,   uint8_t,  gets8le,   decode8le)
DEFGETTER(int16_t,  uint16_t, gets16le,  decode16le)
DEFGETTER(int32_t,  uint32_t, gets32le,  decode32le)
DEFGETTER(float,    uint32_t, getf32le,  decode32le)
DEFGETTER(char,     uint8_t,  getc8le,   decode8le)

DEFSETTER(uint8_t,  uint8_t,  setu8le,   encode8le)
DEFSETTER(uint16_t, uint16_t, setu16le,  encode16le)
DEFSETTER(uint32_t, uint32_t, setu32le,  encode32le)
DEFSETTER(int8_t,   uint8_t,  sets8le,   encode8le)
DEFSETTER(int16_t,  uint16_t, sets16le,  encode16le)
DEFSETTER(int32_t,  uint32_t, sets32le,  encode32le)
DEFSETTER(float,    uint32_t, setf32le,  encode32le)
DEFSETTER(char,     uint8_t,  setc8le,   encode8le)

static inline Vector3f getv3f(uint8_t * const buff, size_t * index) {
    float x = getf32le(buff, index);
    float y = getf32le(buff, index);
    float z = getf32le(buff, index);

    return (Vector3f) {.x = x, .y = y, .z = z};
}

static inline void setv3f(uint8_t * buff, size_t * index, Vector3f vec) {
    setf32le(buff, index, vec.x);
    setf32le(buff, index, vec.y);
    setf32le(buff, index, vec.z);
}

static inline Vector3i getv3i(uint8_t * const buff, size_t * index) {
    uint32_t x = getu32le(buff, index);
    uint32_t y = getu32le(buff, index);
    uint32_t z = getu32le(buff, index);

    return (Vector3i) {.x = x, .y = y, .z = z};
}

static inline void setv3i(uint8_t * const buff, size_t * index, Vector3i vec) {
    setu32le(buff, index, vec.x);
    setu32le(buff, index, vec.y);
    setu32le(buff, index, vec.z);
}

static inline RGB3i getbgr(uint8_t * const buff, size_t * index) {
    uint8_t b = getu8le(buff, index);
    uint8_t g = getu8le(buff, index);
    uint8_t r = getu8le(buff, index);

    return (RGB3i) {.r = r, .g = g, .b = b};
}

static inline void setbgr(uint8_t * const buff, size_t * index, RGB3i color) {
    setu8le(buff, index, color.b);
    setu8le(buff, index, color.g);
    setu8le(buff, index, color.r);
}

static inline RGBA4i getbgra(uint8_t * const buff, size_t * index) {
    uint8_t b = getu8le(buff, index);
    uint8_t g = getu8le(buff, index);
    uint8_t r = getu8le(buff, index);
    uint8_t a = getu8le(buff, index);

    return (RGBA4i) {r, g, b, a};
}

static inline RGBA4i opaque(RGB3i color)
{ return (RGBA4i) {.r = color.r, .g = color.g, .b = color.b, .a = 255}; }

void writeRGBA(uint32_t *, RGBA4i);
void writeBGR(uint32_t *, RGBA4i);

RGBA4i readBGR(uint32_t *);
RGBA4i readBGRA(uint32_t *);

void strnzcpy(char * dest, const char * src, size_t);
size_t strsize(const char *, size_t maxsize);

// QUESTION: should we allow players to change this?
#define RENDER_DISTANCE 128.0F

                        // NOTE: These options are intended for testing purposes only.
                        // NOTE: Don’t cry if you got banned for using this on a public server.
                        // ┌───────────────┬──────────────┬──────────────────────────────────────────────────────────────────────────┐
                        // │ Easy to spot? │ Easy to fix? │ Reason                                                                   │
                        // ├───────────────┼──────────────┼──────────────────────────────────────────────────────────────────────────┤
#define HACK_NORELOAD 0 // │ Yes           │ Kinda        │ Hit packets are not checked for shooting without PacketWeaponInput.      │
#define HACK_NORECOIL 0 // │ For spectator │ No           │ Recoil is client-side.                                                   │
#define HACK_NOSPREAD 0 // │ For spectator │ No           │ Spread is client-side.                                                   │
#define HACK_WALLHACK 0 // │ Yes           │ Yes          │ Hit packets are not checked for shooting through walls.                  │
#define HACK_NOFOG    0 // │ No            │ Impossible?  │ Fog is (totally) client-side.                                            │
#define HACK_MAPHACK  0 // │ No            │ Strongly no  │ Player’s position data is always sent to everyone in full.               │
#define HACK_ESP      0 // │ Kinda         │ Strongly no  │ Same as previous.                                                        │
#define HACK_HEADSHOT 0 // │ Kinda         │ No           │ It’s hard to determine actual headshot in the presence of non-zero ping. │
#define HACK_NOCLIP   0 // │ Yes           │ Yes          │ Some servers don’t check walking in the walls.                           │
                        // └───────────────┴──────────────┴──────────────────────────────────────────────────────────────────────────┘
#define HACKS_ENABLED ((HACK_NORELOAD || HACK_NORECOIL || HACK_NOSPREAD || HACK_WALLHACK || HACK_MAPHACK || HACK_NOFOG || HACK_ESP || HACK_HEADSHOT || HACK_NOCLIP) && 0)

/*
List of other known hacks:
1) Big heads: just make the head model and its hitbox bigger so that aiming becomes *much* easier.
   Fortunately, it is usually easy to spot.
2) Grenade sniping: the governing equations for the flight of a grenade are dv = (0, 0, −1)dt and dr = 32vdt,
   so it’s possible to calculate required angle and fuse time to hit someone’s head exactly (just make
   t′ = 32t substitution and solve the standard free fall problem).
   Relatively easy to detect even without going into spectator mode.
3) Block painting: (vanilla) piqueserver doesn’t check if there was anything in place of the newly placed block
   so you can effectively paint just by allowing the client to place blocks where they already exist.
   Technically it’s still a hack, but it’s unlikely that *this* can ruin the game.
   Very easy to detect and fix.
4) Automatic crouch clicker: not much different from the manual crouch spam but still possible.
5) Skywalking: since blocks can be placed literally behind you (piqueserver doesn’t check this
   because otherwise building with the non-zero ping would be very frustrating), you can automatically
   build a staircase right under your feet.
   Hard to fix but trivial to detect.
6) All sorts of NaN/INF-related problems (that is, just plug an invalid value into your favorite packet):
   these seem to have been fixed in the latest versions of piqueserver.
*/

#endif
