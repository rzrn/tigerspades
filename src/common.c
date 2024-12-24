#include <string.h>

#include <bs/common.h>

const TrueColor White   = {0xFF, 0xFF, 0xFF, 0xFF};
const TrueColor Black   = {0x00, 0x00, 0x00, 0xFF};
const TrueColor Red     = {0xFF, 0x00, 0x00, 0xFF};
const TrueColor Green   = {0x00, 0xFF, 0x00, 0xFF};
const TrueColor Blue    = {0x00, 0x00, 0xFF, 0xFF};
const TrueColor Yellow  = {0xFF, 0xFF, 0x00, 0xFF};
const TrueColor Cyan    = {0x00, 0xFF, 0xFF, 0xFF};
const TrueColor Magenta = {0xFF, 0x00, 0xFF, 0xFF};
const TrueColor Sky     = {0x80, 0xE8, 0xFF, 0xFF};

const RGB3i Gray = {111, 111, 111};

void writeRGBA(uint32_t * dest, TrueColor color) {
    *((uint8_t *) dest + 0) = color.r;
    *((uint8_t *) dest + 1) = color.g;
    *((uint8_t *) dest + 2) = color.b;
    *((uint8_t *) dest + 3) = color.a;
}

void writeBGR(uint32_t * dest, TrueColor color) {
    *((uint8_t *) dest + 0) = color.b;
    *((uint8_t *) dest + 1) = color.g;
    *((uint8_t *) dest + 2) = color.r;
    *((uint8_t *) dest + 3) = 255;
}

TrueColor readBGR(uint32_t * src) {
    TrueColor retval;

    retval.b = *((uint8_t *) src + 0);
    retval.g = *((uint8_t *) src + 1);
    retval.r = *((uint8_t *) src + 2);
    retval.a = 255;

    return retval;
}

TrueColor readBGRA(uint32_t * src) {
    TrueColor retval;

    retval.b = *((uint8_t *) src + 0);
    retval.g = *((uint8_t *) src + 1);
    retval.r = *((uint8_t *) src + 2);
    retval.a = *((uint8_t *) src + 3);

    return retval;
}

void strnzcpy(char * dest, const char * src, size_t size) {
    strncpy(dest, src, size - 1); dest[size - 1] = 0;
}

size_t strsize(const char * buff, size_t maxsize) {
    size_t size = 0;

    while (buff[size] != 0 && size < maxsize)
        size++;

    return size + 1;
}
