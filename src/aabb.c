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

#include <float.h>
#include <stdlib.h>
#include <math.h>

#include <bs/common.h>
#include <bs/aabb.h>
#include <bs/map.h>

// see: https://tavianator.com/2011/ray_box.html
bool aabb_intersection_ray(AABB * a, Ray * r, float * distance) {
    double irx = 1.0 / r->direction[X];
    double tx1 = (a->min[X] - r->origin[X]) * irx;
    double tx2 = (a->max[X] - r->origin[X]) * irx;

    double tmin = fmin(tx1, tx2);
    double tmax = fmax(tx1, tx2);

    double iry = 1.0 / r->direction[Y];
    double ty1 = (a->min[Y] - r->origin[Y]) * iry;
    double ty2 = (a->max[Y] - r->origin[Y]) * iry;

    tmin = fmax(tmin, fmin(fmin(ty1, ty2), tmax));
    tmax = fmin(tmax, fmax(fmax(ty1, ty2), tmin));

    double irz = 1.0 / r->direction[Z];
    double tz1 = (a->min[Z] - r->origin[Z]) * irz;
    double tz2 = (a->max[Z] - r->origin[Z]) * irz;

    tmin = fmax(tmin, fmin(fmin(tz1, tz2), tmax));
    tmax = fmin(tmax, fmax(fmax(tz1, tz2), tmin));

    if (distance != NULL) *distance = fmax(tmin, 0.0) * hypot3f(r->direction[X], r->direction[Y], r->direction[Z]);

    return tmax > fmax(tmin, 0.0);
}

void aabb_set_center(AABB * a, float x, float y, float z) {
    float xsize = a->max[X] - a->min[X];
    float ysize = a->max[Y] - a->min[Y];
    float zsize = a->max[Z] - a->min[Z];

    a->min[X] = x - xsize / 2;
    a->min[Y] = y - ysize / 2;
    a->min[Z] = z - zsize / 2;
    a->max[X] = x + xsize / 2;
    a->max[Y] = y + ysize / 2;
    a->max[Z] = z + zsize / 2;
}

void aabb_set_size(AABB * a, float x, float y, float z) {
    a->max[X] = a->min[X] + x;
    a->max[Y] = a->min[Y] + y;
    a->max[Z] = a->min[Z] + z;
}

bool aabb_intersection(AABB * a, AABB * b) {
    return (a->min[X] <= b->max[X] && b->min[X] <= a->max[X])
        && (a->min[Y] <= b->max[Y] && b->min[Y] <= a->max[Y])
        && (a->min[Z] <= b->max[Z] && b->min[Z] <= a->max[Z]);
}

bool aabb_intersection_terrain(AABB * a) {
    int ymin = clamp(0, map_size_y, floor(a->min[Y]) - 1);
    int ymax = clamp(0, map_size_y, ceil(a->max[Y]) + 1);

    int xmin = floor(a->min[X]) - 1;
    int xmax = ceil(a->max[X]) + 1;

    int zmin = floor(a->min[Z]) - 1;
    int zmax = ceil(a->max[Z]) + 1;

    for (int x = xmin; x < xmax; x++)
    for (int z = zmin; z < zmax; z++)
    for (int y = ymin; y < ymax; y++) {
        if (!map_isair(modnonnegi(x, map_size_x), y, modnonnegi(z, map_size_z))) {
            AABB aabb = {.min = {x, y, z}, .max = {x + 1, y + 1, z + 1}};
            if (aabb_intersection(a, &aabb)) return true;
        }
    }

    return false;
}
