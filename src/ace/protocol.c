#include <AceOfSpades/protocol.h>
#include <BetterSpades/common.h>
#include <string.h>

#define begin(T) size_t read##T(uint8_t * buff, T * contained) { size_t index = 0;
#define end() return index; }

#define c8(dest)      contained->dest = getc8le(buff, &index);
#define u8(dest)      contained->dest = getu8le(buff, &index);
#define u16(dest)     contained->dest = getu16le(buff, &index);
#define u32(dest)     contained->dest = getu32le(buff, &index);
#define f32(dest)     contained->dest = getf32le(buff, &index);
#define v3f(dest)     contained->dest = getv3f(buff, &index);
#define v3i(dest)     contained->dest = getv3i(buff, &index);
#define bgr(dest)     contained->dest = getbgr(buff, &index);
#define string(dest)  contained->dest = (char *) &buff[index];
#define blob(dest, n) contained->dest = (Blob) {.data = &buff[index], .size = n}; index += n;

#include <AceOfSpades/packets.h>

#define begin(T) size_t write##T(uint8_t * buff, T * contained) { size_t index = 0;
#define end() return index; }

#define c8(src)      setc8le(buff, &index, contained->src);
#define u8(src)      setu8le(buff, &index, contained->src);
#define u16(src)     setu16le(buff, &index, contained->src);
#define u32(src)     setu32le(buff, &index, contained->src);
#define f32(src)     setf32le(buff, &index, contained->src);
#define v3f(src)     setv3f(buff, &index, contained->src);
#define v3i(src)     setv3i(buff, &index, contained->src);
#define bgr(src)     setbgr(buff, &index, contained->src);
#define string(src)  strcpy((char *) &buff[index], contained->src);
#define blob(src, n) memcpy(&buff[index], contained->src.data, n); index += n;

#include <AceOfSpades/packets.h>

#define begin(T) const size_t size##T = 0
#define end() ;

#define c8(dest)      + 1
#define u8(dest)      + 1
#define u16(dest)     + 2
#define u32(dest)     + 4
#define f32(dest)     + 4
#define v3f(dest)     + 12
#define v3i(dest)     + 12
#define bgr(dest)     + 3
#define string(dest)  + 0
#define blob(dest, n) + n

#include <AceOfSpades/packets.h>
