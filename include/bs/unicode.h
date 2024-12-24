#ifndef UNICODE_H
#define UNICODE_H

#include <BetterSpades/common.h>

#define OCT1(c) ((((uint8_t) c) & 0x80) == 0x00)
#define OCT2(c) ((((uint8_t) c) & 0xE0) == 0xC0)
#define OCT3(c) ((((uint8_t) c) & 0xF0) == 0xE0)
#define OCT4(c) ((((uint8_t) c) & 0xF8) == 0xF0)
#define CONT(c) ((((uint8_t) c) & 0xC0) == 0x80)

uint8_t encodeSize(Codepage, uint32_t);
void encode(Codepage, uint8_t *, uint32_t);

uint8_t decodeSize(Codepage, const uint8_t);
uint32_t decode(Codepage, const uint8_t *);

void convert(char * dest, size_t outsize, Codepage outpage,
             const char * src, size_t insize, Codepage inpage);

size_t encodeMagic(char * dest, size_t outsize, const char * src, size_t insize);
void decodeMagic(char * dest, size_t outsize, const char * src, size_t insize);

#endif
