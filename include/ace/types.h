/*
    Copyright © 2024–2025 rzrn

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

#ifndef ACE_TYPES_H
#define ACE_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    int x, y;
} Vector2i;

typedef struct {
    float x, y;
} Vector2f;

typedef struct {
    int x, y, z;
} Vector3i;

typedef struct {
    float x, y, z;
} Vector3f;

typedef struct {
    uint8_t r, g, b;
} RGB3i;

typedef struct {
    float r, g, b;
} RGB3f;

typedef struct {
    uint8_t r, g, b, a;
} RGBA4i;

typedef struct {
    uint8_t * data; size_t size;
} Blob;

typedef float float2[2];
typedef float float3[3];
typedef float float4[4];

#endif
