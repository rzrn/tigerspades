#ifndef AOS_TYPES_H
#define AOS_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    float x, y, z;
} Vector3f;

typedef struct {
    int x, y, z;
} Vector3i;

typedef struct {
    float r, g, b;
} RGB3f;

typedef struct {
    uint8_t r, g, b;
} RGB3i;

typedef struct {
    uint8_t r, g, b, a;
} TrueColor;

typedef struct {
    uint8_t * data; size_t size;
} Blob;

#endif
