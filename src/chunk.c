/*
    Copyright © 2016–2021 ByteBit
    Copyright © 2018 vuolen
    Copyright © 2023–2026 rzrn

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

#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

#include <bs/common.h>
#include <bs/window.h>
#include <bs/config.h>
#include <bs/texture.h>
#include <bs/matrix.h>
#include <bs/map.h>
#include <bs/camera.h>
#include <bs/tesselator.h>
#include <bs/chunk.h>
#include <bs/channel.h>
#include <bs/utils.h>

#include <log.h>

float frustum_near(void) { return 0.1F;                                         }
float frustum_far(void)  { return settings.render_distance + CHUNK_SIZE * 4.0F; }

Chunk chunks[CHUNKS_PER_DIM * CHUNKS_PER_DIM];

HashTable chunk_block_queue;
Channel chunk_work_queue, chunk_result_queue;
pthread_mutex_t chunk_block_queue_lock;

typedef struct {
    size_t chunk_x;
    size_t chunk_y;
    Chunk * chunk;
} ChunkWorkPacket;

typedef struct {
    Chunk * chunk;
    int max_height;
    Tesselator tesselator;
    uint32_t * minimap_data;
} ChunkResultPacket;

typedef struct {
    Chunk * chunk;
    int mirror_x;
    int mirror_y;
} ChunkRenderCall;

void chunk_init(void) {
    for (size_t x = 0; x < CHUNKS_PER_DIM; x++) {
        for (size_t y = 0; y < CHUNKS_PER_DIM; y++) {
            Chunk * c = chunks + x + y * CHUNKS_PER_DIM;
            c->created = false;
            c->max_height = 1;
            c->x = x;
            c->y = y;
        }
    }

    channel_create(&chunk_work_queue, sizeof(ChunkWorkPacket), CHUNKS_PER_DIM * CHUNKS_PER_DIM);
    channel_create(&chunk_result_queue, sizeof(ChunkResultPacket), CHUNKS_PER_DIM * CHUNKS_PER_DIM);
    ht_setup(&chunk_block_queue, sizeof(Chunk *), sizeof(ChunkResultPacket), 64);

    pthread_mutex_init(&chunk_block_queue_lock, NULL);

    size_t chunk_enabled_cores = clamp(1, CHUNK_WORKERS_MAX, window_cpucores() / 2);
    log_info("%i cores enabled for chunk generation", chunk_enabled_cores);

    for (size_t k = 0; k < chunk_enabled_cores; k++)
    { pthread_t _thread; pthread_create(&_thread, NULL, chunk_generate, NULL); }
}

static int chunk_sort(const void * a, const void * b) {
    ChunkRenderCall * aa = (ChunkRenderCall *) a;
    ChunkRenderCall * bb = (ChunkRenderCall *) b;
    return norm2f(aa->chunk->x * CHUNK_SIZE + CHUNK_SIZE / 2, aa->chunk->y * CHUNK_SIZE + CHUNK_SIZE / 2, camera.r.x, camera.r.z)
         - norm2f(bb->chunk->x * CHUNK_SIZE + CHUNK_SIZE / 2, bb->chunk->y * CHUNK_SIZE + CHUNK_SIZE / 2, camera.r.x, camera.r.z);
}

void chunk_render(ChunkRenderCall * c) {
    if (c->chunk->created) {
        matrix_push(matrix_model);
        matrix_translate(matrix_model, c->mirror_x * map_size_x, 0.0F, c->mirror_y * map_size_z);
        matrix_upload();

        //glPolygonMode(GL_FRONT, GL_LINE);

        glx_displaylist_draw(&c->chunk->display_list, GLX_DISPLAYLIST_NORMAL);

        //glPolygonMode(GL_FRONT, GL_FILL);

        matrix_pop(matrix_model);
    }
}

void chunk_draw_visible(void) {
    ChunkRenderCall chunks_draw[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
    int index = 0;

    int overshoot = (settings.render_distance + CHUNK_SIZE - 1) / CHUNK_SIZE + 1;

    // go through all possible chunks and store all in range and view
    for (int y = -overshoot; y < CHUNKS_PER_DIM + overshoot; y++) {
        for (int x = -overshoot; x < CHUNKS_PER_DIM + overshoot; x++) {
            if (norm2f((x + 0.5F) * CHUNK_SIZE, (y + 0.5F) * CHUNK_SIZE, camera.r.x, camera.r.z) <=
                sqrf(settings.render_distance + 1.414F * CHUNK_SIZE)) {
                uint32_t tmp_x = ((uint32_t) x) % CHUNKS_PER_DIM;
                uint32_t tmp_y = ((uint32_t) y) % CHUNKS_PER_DIM;

                Chunk * c = chunks + tmp_x + tmp_y * CHUNKS_PER_DIM;

                if (camera_CubeInFrustum((x + 0.5F) * CHUNK_SIZE, 0.0F, (y + 0.5F) * CHUNK_SIZE, CHUNK_SIZE / 2,
                                        c->max_height))
                    chunks_draw[index++] = (ChunkRenderCall) {
                        .chunk = c,
                        .mirror_x = (x < 0) ? -1 : ((x >= CHUNKS_PER_DIM) ? 1 : 0),
                        .mirror_y = (y < 0) ? -1 : ((y >= CHUNKS_PER_DIM) ? 1 : 0),
                    };
            }
        }
    }

    // sort all chunks to draw those in front first
    qsort(chunks_draw, index, sizeof(ChunkRenderCall), chunk_sort);

    for (int k = 0; k < index; k++)
        chunk_render(chunks_draw + k);
}

__attribute__((always_inline))
static inline bool solid_array_isair(struct libvxl_chunk_copy * blocks,
                                     uint32_t x, int32_t y, uint32_t z) {
    if (y < 0)
        return false;
    if (y >= map_size_y)
        return true;

    return !libvxl_copy_chunk_is_solid(blocks, x % map_size_x, z % map_size_z, map_size_y - 1 - y);
}

__attribute__((always_inline))
static inline float solid_sunblock(struct libvxl_chunk_copy * blocks, int x, int y, int z) {
    int dec = 18;
    int i = 127;

    while (dec && y < map_size_y) {
        if (z == 0) break;

        if (!solid_array_isair(blocks, x, ++y, --z))
            i -= dec;

        dec -= 2;
    }

    return (float) i / 127.0F;
}

void * chunk_generate(void * data) {
    UNUSED(data);

    pthread_detach(pthread_self());

    while (1) {
        ChunkWorkPacket work;
        channel_await(&chunk_work_queue, &work);

        if (!work.chunk) break;

        ChunkResultPacket result;
        result.chunk = work.chunk;
        result.minimap_data = malloc(CHUNK_SIZE * CHUNK_SIZE * sizeof(uint32_t));
        tesselator_create(&result.tesselator, VERTEX_INT, 0);

        struct libvxl_chunk_copy blocks;
        map_copy_blocks(&blocks, work.chunk_x * CHUNK_SIZE, work.chunk_y * CHUNK_SIZE);

        if (settings.greedy_meshing)
            chunk_generate_greedy(&blocks, work.chunk_x * CHUNK_SIZE, work.chunk_y * CHUNK_SIZE, &result.tesselator,
                                  &result.max_height);
        else
            chunk_generate_naive(&blocks, &result.tesselator, &result.max_height, settings.ambient_occlusion);

        // Use the fact that libvxl orders libvxl_blocks by top-down coordinate first in its data structure.
        size_t chunk_x = work.chunk_x * CHUNK_SIZE;
        size_t chunk_y = work.chunk_y * CHUNK_SIZE;
        uint32_t last_position = 0;

        // Clean up garbage contained in “result.minimap_data”.
        for (int i = 0; i < CHUNK_SIZE; i++) {
            for (int j = 0; j < CHUNK_SIZE; j++) {
                int x = i + chunk_x, z = j + chunk_y;
                uint32_t * out = result.minimap_data + i + j * CHUNK_SIZE;
                writeRGBA(out, ISGRID(x, z) ? White : Sky);
            }
        }

        for (int k = blocks.blocks_sorted_count - 1; k >= 0; k--) {
            struct libvxl_block * blk = blocks.blocks_sorted + k;

            if (blk->position != last_position || k == blocks.blocks_sorted_count - 1) {
                last_position = blk->position;

                int x = key_getx(blk->position), z = key_gety(blk->position);
                uint32_t * out = result.minimap_data + (x - chunk_x) + (z - chunk_y) * CHUNK_SIZE;
                writeRGBA(out, ISGRID(x, z) ? White : readBGR(&blk->color));
            }
        }

        libvxl_copy_chunk_destroy(&blocks);

        channel_put(&chunk_result_queue, &result);
    }

    return NULL;
}

void chunk_generate_greedy(struct libvxl_chunk_copy * blocks, size_t start_x, size_t start_z, Tesselator * tess,
                           int * max_height) {
    *max_height = 0;

    int checked_voxels[2][CHUNK_SIZE * CHUNK_SIZE];
    int checked_voxels2[2][CHUNK_SIZE * map_size_y];

    for (int z = start_z; z < start_z + CHUNK_SIZE; z++) {
        memset(checked_voxels2[0], 0, sizeof(int) * CHUNK_SIZE * map_size_y);
        memset(checked_voxels2[1], 0, sizeof(int) * CHUNK_SIZE * map_size_y);

        for (int x = start_x; x < start_x + CHUNK_SIZE; x++) {
            for (int y = 0; y < map_size_y; y++) {
                if (!solid_array_isair(blocks, x, y, z)) {
                    if (*max_height < y)
                        *max_height = y;

                    uint32_t value = libvxl_copy_chunk_get_color(blocks, x, z, map_size_y - 1 - y);
                    RGBA4i color = readBGR(&value);

                    if ((z == 0 && solid_array_isair(blocks, x, y, map_size_z - 1))
                       || (z > 0 && solid_array_isair(blocks, x, y, z - 1))) {
                        if (checked_voxels2[0][y + (x - start_x) * map_size_y] == 0) {
                            int len_y = 1;
                            int len_x = 1;

                            for (int a = 1; a < map_size_y - y; a++) {
                                if (!solid_array_isair(blocks, x, y + a, z)
                                   && libvxl_copy_chunk_get_color(blocks, x, z, map_size_y - 1 - (y + a)) == value
                                   && checked_voxels2[0][y + a + (x - start_x) * map_size_y] == 0
                                   && ((z == 0 && solid_array_isair(blocks, x, y + a, map_size_z - 1))
                                       || (z > 0 && solid_array_isair(blocks, x, y + a, z - 1))))
                                    len_y++;
                                else
                                    break;
                            }

                            for (int b = 1; b < (start_x + CHUNK_SIZE - x); b++) {
                                int a;
                                for (a = 0; a < len_y; a++) {
                                    if (solid_array_isair(blocks, x + b, y + a, z)
                                       || libvxl_copy_chunk_get_color(blocks, x + b, z, map_size_y - 1 - (y + a)) != value
                                       || checked_voxels2[0][y + a + (x + b - start_x) * map_size_y] != 0
                                       || !((z == 0 && solid_array_isair(blocks, x + b, y + a, map_size_z - 1))
                                            || (z > 0 && solid_array_isair(blocks, x + b, y + a, z - 1))))
                                        break;
                                }
                                if (a == len_y)
                                    len_x++;
                                else
                                    break;
                            }

                            for (int b = 0; b < len_x; b++)
                                for (int a = 0; a < len_y; a++)
                                    checked_voxels2[0][y + a + (x + b - start_x) * map_size_y] = 1;

                            tesselator_set_color(tess, (RGBA4i) {color.r * 0.875F, color.g * 0.875F, color.b * 0.875F, 255});
                            tesselator_addi_simple(
                                tess, (int16_t[]) {x, y, z, x, y + len_y, z, x + len_x, y + len_y, z, x + len_x, y, z});
                        }
                    }

                    if ((z == map_size_z - 1 && solid_array_isair(blocks, x, y, 0))
                       || (z < map_size_z - 1 && solid_array_isair(blocks, x, y, z + 1))) {
                        if (checked_voxels2[1][y + (x - start_x) * map_size_y] == 0) {
                            int len_y = 1;
                            int len_x = 1;

                            for (int a = 1; a < map_size_y - y; a++) {
                                if (!solid_array_isair(blocks, x, y + a, z)
                                   && libvxl_copy_chunk_get_color(blocks, x, z, map_size_y - 1 - (y + a)) == value
                                   && checked_voxels2[1][y + a + (x - start_x) * map_size_y] == 0
                                   && ((z == map_size_z - 1 && solid_array_isair(blocks, x, y + a, 0))
                                       || (z < map_size_z - 1 && solid_array_isair(blocks, x, y + a, z + 1))))
                                    len_y++;
                                else
                                    break;
                            }

                            for (int b = 1; b < (start_x + CHUNK_SIZE - x); b++) {
                                int a;
                                for (a = 0; a < len_y; a++) {
                                    if (solid_array_isair(blocks, x + b, y + a, z)
                                       || libvxl_copy_chunk_get_color(blocks, x + b, z, map_size_y - 1 - (y + a)) != value
                                       || checked_voxels2[1][y + a + (x + b - start_x) * map_size_y] != 0
                                       || !((z == map_size_z - 1 && solid_array_isair(blocks, x + b, y + a, 0))
                                            || (z < map_size_z - 1 && solid_array_isair(blocks, x + b, y + a, z + 1))))
                                        break;
                                }
                                if (a == len_y)
                                    len_x++;
                                else
                                    break;
                            }

                            for (int b = 0; b < len_x; b++)
                                for (int a = 0; a < len_y; a++)
                                    checked_voxels2[1][y + a + (x + b - start_x) * map_size_y] = 1;

                            tesselator_set_color(tess, (RGBA4i) {color.r * 0.625F, color.g * 0.625F, color.b * 0.625F, 255});
                            tesselator_addi_simple(tess,
                                                   (int16_t[]) {x, y, z + 1, x + len_x, y, z + 1, x + len_x, y + len_y,
                                                                z + 1, x, y + len_y, z + 1});
                        }
                    }
                }
            }
        }
    }

    for (int x = start_x; x < start_x + CHUNK_SIZE; x++) {
        memset(checked_voxels2[0], 0, sizeof(int) * CHUNK_SIZE * map_size_y);
        memset(checked_voxels2[1], 0, sizeof(int) * CHUNK_SIZE * map_size_y);

        for (int z = start_z; z < start_z + CHUNK_SIZE; z++) {
            for (int y = 0; y < map_size_y; y++) {
                if (!solid_array_isair(blocks, x, y, z)) {
                    if (*max_height < y) {
                        *max_height = y;
                    }

                    uint32_t value = libvxl_copy_chunk_get_color(blocks, x, z, map_size_y - 1 - y);
                    RGBA4i color = readBGR(&value);

                    if ((x == 0 && solid_array_isair(blocks, map_size_x - 1, y, z))
                       || (x > 0 && solid_array_isair(blocks, x - 1, y, z))) {
                        if (checked_voxels2[0][y + (z - start_z) * map_size_y] == 0) {
                            int len_y = 1;
                            int len_z = 1;

                            for (int a = 1; a < map_size_y - y; a++) {
                                if (!solid_array_isair(blocks, x, y + a, z)
                                   && libvxl_copy_chunk_get_color(blocks, x, z, map_size_y - 1 - (y + a)) == value
                                   && checked_voxels2[0][y + a + (z - start_z) * map_size_y] == 0
                                   && ((x == 0 && solid_array_isair(blocks, map_size_x - 1, y + a, z))
                                       || (x > 0 && solid_array_isair(blocks, x - 1, y + a, z))))
                                    len_y++;
                                else
                                    break;
                            }

                            for (int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
                                int a;
                                for (a = 0; a < len_y; a++) {
                                    if (solid_array_isair(blocks, x, y + a, z + b)
                                       || libvxl_copy_chunk_get_color(blocks, x, z + b, map_size_y - 1 - (y + a)) != value
                                       || checked_voxels2[0][y + a + (z + b - start_z) * map_size_y] != 0
                                       || !((x == 0 && solid_array_isair(blocks, map_size_x - 1, y + a, z + b))
                                            || (x > 0 && solid_array_isair(blocks, x - 1, y + a, z + b))))
                                        break;
                                }
                                if (a == len_y)
                                    len_z++;
                                else
                                    break;
                            }

                            for (int b = 0; b < len_z; b++)
                                for (int a = 0; a < len_y; a++)
                                    checked_voxels2[0][y + a + (z + b - start_z) * map_size_y] = 1;

                            tesselator_set_color(tess, (RGBA4i) {color.r * 0.75F, color.g * 0.75F, color.b * 0.75F, 255});
                            tesselator_addi_simple(
                                tess, (int16_t[]) {x, y, z, x, y, z + len_z, x, y + len_y, z + len_z, x, y + len_y, z});
                        }
                    }

                    if ((x == map_size_x - 1 && solid_array_isair(blocks, 0, y, z))
                       || (x < map_size_x - 1 && solid_array_isair(blocks, x + 1, y, z))) {
                        if (checked_voxels2[1][y + (z - start_z) * map_size_y] == 0) {
                            int len_y = 1, len_z = 1;

                            for (int a = 1; a < map_size_y - y; a++) {
                                if (!solid_array_isair(blocks, x, y + a, z)
                                   && libvxl_copy_chunk_get_color(blocks, x, z, map_size_y - 1 - (y + a)) == value
                                   && checked_voxels2[1][y + a + (z - start_z) * map_size_y] == 0
                                   && ((x == map_size_x - 1 && solid_array_isair(blocks, 0, y + a, z))
                                       || (x < map_size_x - 1 && solid_array_isair(blocks, x + 1, y + a, z))))
                                    len_y++;
                                else
                                    break;
                            }

                            for (int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
                                int a;
                                for (a = 0; a < len_y; a++) {
                                    if (solid_array_isair(blocks, x, y + a, z + b)
                                       || libvxl_copy_chunk_get_color(blocks, x, z + b, map_size_y - 1 - (y + a)) != value
                                       || checked_voxels2[1][y + a + (z + b - start_z) * map_size_y] != 0
                                       || !((x == map_size_x - 1 && solid_array_isair(blocks, 0, y + a, z + b))
                                            || (x < map_size_x - 1 && solid_array_isair(blocks, x + 1, y + a, z + b))))
                                        break;
                                }
                                if (a == len_y)
                                    len_z++;
                                else
                                    break;
                            }

                            for (unsigned char b = 0; b < len_z; b++)
                                for (unsigned char a = 0; a < len_y; a++)
                                    checked_voxels2[1][y + a + (z + b - start_z) * map_size_y] = 1;

                            tesselator_set_color(tess, (RGBA4i) {color.r * 0.75F, color.g * 0.75F, color.b * 0.75F, 255});
                            tesselator_addi_simple(tess,
                                                   (int16_t[]) {x + 1, y, z, x + 1, y + len_y, z, x + 1, y + len_y,
                                                                z + len_z, x + 1, y, z + len_z});
                        }
                    }
                }
            }
        }
    }

    for (int y = 0; y < map_size_y; y++) {
        memset(checked_voxels[0], 0, sizeof(int) * CHUNK_SIZE * CHUNK_SIZE);
        memset(checked_voxels[1], 0, sizeof(int) * CHUNK_SIZE * CHUNK_SIZE);

        for (int x = start_x; x < start_x + CHUNK_SIZE; x++) {
            for (int z = start_z; z < start_z + CHUNK_SIZE; z++) {
                if (!solid_array_isair(blocks, x, y, z)) {
                    if (*max_height < y)
                        *max_height = y;

                    uint32_t value = libvxl_copy_chunk_get_color(blocks, x, z, map_size_y - 1 - y);
                    RGBA4i color = readBGR(&value);

                    if (y == map_size_y - 1 || solid_array_isair(blocks, x, y + 1, z)) {
                        if (checked_voxels[0][(x - start_x) + (z - start_z) * CHUNK_SIZE] == 0) {
                            int len_x = 1;
                            int len_z = 1;

                            for (int a = 1; a < (start_x + CHUNK_SIZE - x); a++) {
                                if (!solid_array_isair(blocks, x + a, y, z)
                                   && libvxl_copy_chunk_get_color(blocks, x + a, z, map_size_y - 1 - y) == value
                                   && checked_voxels[0][(x + a - start_x) + (z - start_z) * CHUNK_SIZE] == 0
                                   && (y == map_size_y - 1 || solid_array_isair(blocks, x + a, y + 1, z)))
                                    len_x++;
                                else
                                    break;
                            }

                            for (int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
                                int a;
                                for (a = 0; a < len_x; a++) {
                                    if (solid_array_isair(blocks, x + a, y, z + b)
                                       || libvxl_copy_chunk_get_color(blocks, x + a, z + b, map_size_y - 1 - y) != value
                                       || checked_voxels[0][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] != 0
                                       || !(y == map_size_y - 1 || solid_array_isair(blocks, x + a, y + 1, z + b)))
                                        break;
                                }
                                if (a == len_x)
                                    len_z++;
                                else
                                    break;
                            }

                            for (int b = 0; b < len_z; b++)
                                for (int a = 0; a < len_x; a++)
                                    checked_voxels[0][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] = 1;

                            tesselator_set_color(tess, color);
                            tesselator_addi_simple(tess,
                                                   (int16_t[]) {x, y + 1, z, x, y + 1, z + len_z, x + len_x, y + 1,
                                                                z + len_z, x + len_x, y + 1, z});
                        }
                    }

                    if (y > 0 && solid_array_isair(blocks, x, y - 1, z)) {
                        if (checked_voxels[1][(x - start_x) + (z - start_z) * CHUNK_SIZE] == 0) {
                            int len_x = 1;
                            int len_z = 1;

                            for (int a = 1; a < (start_x + CHUNK_SIZE - x); a++) {
                                if (!solid_array_isair(blocks, x + a, y, z)
                                   && libvxl_copy_chunk_get_color(blocks, x + a, z, map_size_y - 1 - y) == value
                                   && checked_voxels[1][(x + a - start_x) + (z - start_z) * CHUNK_SIZE] == 0
                                   && (y > 0 && solid_array_isair(blocks, x + a, y - 1, z)))
                                    len_x++;
                                else
                                    break;
                            }

                            for (int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
                                int a;
                                for (a = 0; a < len_x; a++) {
                                    if (solid_array_isair(blocks, x + a, y, z + b)
                                       || libvxl_copy_chunk_get_color(blocks, x + a, z + b, map_size_y - 1 - y) != value
                                       || checked_voxels[1][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] != 0
                                       || !(y > 0 && solid_array_isair(blocks, x + a, y - 1, z + b)))
                                        break;
                                }
                                if (a == len_x)
                                    len_z++;
                                else
                                    break;
                            }

                            for (int b = 0; b < len_z; b++)
                                for (int a = 0; a < len_x; a++)
                                    checked_voxels[1][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] = 1;

                            tesselator_set_color(tess, (RGBA4i) {color.r * 0.5F, color.g * 0.5F, color.b * 0.5F, 255});
                            tesselator_addi_simple(
                                tess, (int16_t[]) {x, y, z, x + len_x, y, z, x + len_x, y, z + len_z, x, y, z + len_z});
                        }
                    }
                }
            }
        }
    }

    (*max_height)++;
}

//+X = 0.75
//-X = 0.75
//+Y = 1.0
//-Y = 0.5
//+Z = 0.625
//-Z = 0.875

// credit: https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
static float vertexAO(int side1, int side2, int corner) {
    if (!side1 && !side2) return 0.25F;
    return 0.75F - (!side1 + !side2 + !corner) * 0.25F + 0.25F;
}

void chunk_generate_naive(struct libvxl_chunk_copy * blocks, Tesselator * tess, int * max_height, int ao) {
    *max_height = 0;

    for (size_t k = 0; k < blocks->blocks_sorted_count; k++) {
        struct libvxl_block * blk = blocks->blocks_sorted + k;

        int x = key_getx(blk->position);
        int y = map_size_y - 1 - key_getz(blk->position);
        int z = key_gety(blk->position);

        *max_height = max(*max_height, y);

        RGBA4i color = readBGR(&blk->color);

        if (settings.enable_shadows) {
            float shade = solid_sunblock(blocks, x, y, z);
            color.r *= shade; color.g *= shade; color.b *= shade;
        }

        if (z <= 0 || solid_array_isair(blocks, x, y, z - 1)) {
            if (ao) {
                float A
                    = vertexAO(solid_array_isair(blocks, x - 1, y, z - 1), solid_array_isair(blocks, x, y - 1, z - 1),
                               solid_array_isair(blocks, x - 1, y - 1, z - 1));
                float B
                    = vertexAO(solid_array_isair(blocks, x - 1, y, z - 1), solid_array_isair(blocks, x, y + 1, z - 1),
                               solid_array_isair(blocks, x - 1, y + 1, z - 1));
                float C
                    = vertexAO(solid_array_isair(blocks, x + 1, y, z - 1), solid_array_isair(blocks, x, y + 1, z - 1),
                               solid_array_isair(blocks, x + 1, y + 1, z - 1));
                float D
                    = vertexAO(solid_array_isair(blocks, x + 1, y, z - 1), solid_array_isair(blocks, x, y - 1, z - 1),
                               solid_array_isair(blocks, x + 1, y - 1, z - 1));

                tesselator_addi(tess, (int16_t[]) {x, y, z, x, y + 1, z, x + 1, y + 1, z, x + 1, y, z},
                                (RGBA4i[]) {
                                    {color.r * 0.875F * A, color.g * 0.875F * A, color.b * 0.875F * A, 255},
                                    {color.r * 0.875F * B, color.g * 0.875F * B, color.b * 0.875F * B, 255},
                                    {color.r * 0.875F * C, color.g * 0.875F * C, color.b * 0.875F * C, 255},
                                    {color.r * 0.875F * D, color.g * 0.875F * D, color.b * 0.875F * D, 255},
                                },
                                NULL);
            } else {
                tesselator_set_color(tess, (RGBA4i) {color.r * 0.875F, color.g * 0.875F, color.b * 0.875F, 255});
                tesselator_addi_cube_face(tess, CUBE_FACE_Z_N, x, y, z);
            }
        }

        if (z >= map_size_z - 1 || solid_array_isair(blocks, x, y, z + 1)) {
            if (ao) {
                float A
                    = vertexAO(solid_array_isair(blocks, x - 1, y, z + 1), solid_array_isair(blocks, x, y - 1, z + 1),
                               solid_array_isair(blocks, x - 1, y - 1, z + 1));
                float B
                    = vertexAO(solid_array_isair(blocks, x + 1, y, z + 1), solid_array_isair(blocks, x, y - 1, z + 1),
                               solid_array_isair(blocks, x + 1, y - 1, z + 1));
                float C
                    = vertexAO(solid_array_isair(blocks, x + 1, y, z + 1), solid_array_isair(blocks, x, y + 1, z + 1),
                               solid_array_isair(blocks, x + 1, y + 1, z + 1));
                float D
                    = vertexAO(solid_array_isair(blocks, x - 1, y, z + 1), solid_array_isair(blocks, x, y + 1, z + 1),
                               solid_array_isair(blocks, x - 1, y + 1, z + 1));
                tesselator_addi(tess, (int16_t[]) {x, y, z + 1, x + 1, y, z + 1, x + 1, y + 1, z + 1, x, y + 1, z + 1},
                                (RGBA4i[]) {
                                    {color.r * 0.625F * A, color.g * 0.625F * A, color.b * 0.625F * A, 255},
                                    {color.r * 0.625F * B, color.g * 0.625F * B, color.b * 0.625F * B, 255},
                                    {color.r * 0.625F * C, color.g * 0.625F * C, color.b * 0.625F * C, 255},
                                    {color.r * 0.625F * D, color.g * 0.625F * D, color.b * 0.625F * D, 255},
                                },
                                NULL);
            } else {
                tesselator_set_color(tess, (RGBA4i) {color.r * 0.625F, color.g * 0.625F, color.b * 0.625F, 255});
                tesselator_addi_cube_face(tess, CUBE_FACE_Z_P, x, y, z);
            }
        }

        if (x <= 0 || solid_array_isair(blocks, x - 1, y, z)) {
            if (ao) {
                float A
                    = vertexAO(solid_array_isair(blocks, x - 1, y - 1, z), solid_array_isair(blocks, x - 1, y, z - 1),
                               solid_array_isair(blocks, x - 1, y - 1, z - 1));
                float B
                    = vertexAO(solid_array_isair(blocks, x - 1, y - 1, z), solid_array_isair(blocks, x - 1, y, z + 1),
                               solid_array_isair(blocks, x - 1, y - 1, z + 1));
                float C
                    = vertexAO(solid_array_isair(blocks, x - 1, y + 1, z), solid_array_isair(blocks, x - 1, y, z + 1),
                               solid_array_isair(blocks, x - 1, y + 1, z + 1));
                float D
                    = vertexAO(solid_array_isair(blocks, x - 1, y + 1, z), solid_array_isair(blocks, x - 1, y, z - 1),
                               solid_array_isair(blocks, x - 1, y + 1, z - 1));

                tesselator_addi(tess, (int16_t[]) {x, y, z, x, y, z + 1, x, y + 1, z + 1, x, y + 1, z},
                                (RGBA4i[]) {
                                    {color.r * 0.75F * A, color.g * 0.75F * A, color.b * 0.75F * A, 255},
                                    {color.r * 0.75F * B, color.g * 0.75F * B, color.b * 0.75F * B, 255},
                                    {color.r * 0.75F * C, color.g * 0.75F * C, color.b * 0.75F * C, 255},
                                    {color.r * 0.75F * D, color.g * 0.75F * D, color.b * 0.75F * D, 255},
                                },
                                NULL);
            } else {
                tesselator_set_color(tess, (RGBA4i) {color.r * 0.75F, color.g * 0.75F, color.b * 0.75F, 255});
                tesselator_addi_cube_face(tess, CUBE_FACE_X_N, x, y, z);
            }
        }

        if (x >= map_size_x - 1 || solid_array_isair(blocks, x + 1, y, z)) {
            if (ao) {
                float A
                    = vertexAO(solid_array_isair(blocks, x + 1, y - 1, z), solid_array_isair(blocks, x + 1, y, z - 1),
                               solid_array_isair(blocks, x + 1, y - 1, z - 1));
                float B
                    = vertexAO(solid_array_isair(blocks, x + 1, y + 1, z), solid_array_isair(blocks, x + 1, y, z - 1),
                               solid_array_isair(blocks, x + 1, y + 1, z - 1));
                float C
                    = vertexAO(solid_array_isair(blocks, x + 1, y + 1, z), solid_array_isair(blocks, x + 1, y, z + 1),
                               solid_array_isair(blocks, x + 1, y + 1, z + 1));
                float D
                    = vertexAO(solid_array_isair(blocks, x + 1, y - 1, z), solid_array_isair(blocks, x + 1, y, z + 1),
                               solid_array_isair(blocks, x + 1, y - 1, z + 1));

                tesselator_addi(tess, (int16_t[]) {x + 1, y, z, x + 1, y + 1, z, x + 1, y + 1, z + 1, x + 1, y, z + 1},
                                (RGBA4i[]) {
                                    {color.r * 0.75F * A, color.g * 0.75F * A, color.b * 0.75F * A, 255},
                                    {color.r * 0.75F * B, color.g * 0.75F * B, color.b * 0.75F * B, 255},
                                    {color.r * 0.75F * C, color.g * 0.75F * C, color.b * 0.75F * C, 255},
                                    {color.r * 0.75F * D, color.g * 0.75F * D, color.b * 0.75F * D, 255},
                                },
                                NULL);
            } else {
                tesselator_set_color(tess, (RGBA4i) {color.r * 0.75F, color.g * 0.75F, color.b * 0.75F, 255});
                tesselator_addi_cube_face(tess, CUBE_FACE_X_P, x, y, z);
            }
        }

        if (y >= map_size_y - 1 || solid_array_isair(blocks, x, y + 1, z)) {
            if (ao) {
                float A
                    = vertexAO(solid_array_isair(blocks, x - 1, y + 1, z), solid_array_isair(blocks, x, y + 1, z - 1),
                               solid_array_isair(blocks, x - 1, y + 1, z - 1));
                float B
                    = vertexAO(solid_array_isair(blocks, x - 1, y + 1, z), solid_array_isair(blocks, x, y + 1, z + 1),
                               solid_array_isair(blocks, x - 1, y + 1, z + 1));
                float C
                    = vertexAO(solid_array_isair(blocks, x + 1, y + 1, z), solid_array_isair(blocks, x, y + 1, z + 1),
                               solid_array_isair(blocks, x + 1, y + 1, z + 1));
                float D
                    = vertexAO(solid_array_isair(blocks, x + 1, y + 1, z), solid_array_isair(blocks, x, y + 1, z - 1),
                               solid_array_isair(blocks, x + 1, y + 1, z - 1));

                tesselator_addi(tess, (int16_t[]) {x, y + 1, z, x, y + 1, z + 1, x + 1, y + 1, z + 1, x + 1, y + 1, z},
                                (RGBA4i[]) {
                                    {color.r * A, color.g * A, color.b * A, 255},
                                    {color.r * B, color.g * B, color.b * B, 255},
                                    {color.r * C, color.g * C, color.b * C, 255},
                                    {color.r * D, color.g * D, color.b * D, 255},
                                },
                                NULL);
            } else {
                tesselator_set_color(tess, color);
                tesselator_addi_cube_face(tess, CUBE_FACE_Y_P, x, y, z);
            }
        }

        if (y > 0 && solid_array_isair(blocks, x, y - 1, z)) {
            if (ao) {
                float A
                    = vertexAO(solid_array_isair(blocks, x - 1, y - 1, z), solid_array_isair(blocks, x, y - 1, z - 1),
                               solid_array_isair(blocks, x - 1, y - 1, z - 1));
                float B
                    = vertexAO(solid_array_isair(blocks, x + 1, y - 1, z), solid_array_isair(blocks, x, y - 1, z - 1),
                               solid_array_isair(blocks, x + 1, y - 1, z - 1));
                float C
                    = vertexAO(solid_array_isair(blocks, x + 1, y - 1, z), solid_array_isair(blocks, x, y - 1, z + 1),
                               solid_array_isair(blocks, x + 1, y - 1, z + 1));
                float D
                    = vertexAO(solid_array_isair(blocks, x - 1, y - 1, z), solid_array_isair(blocks, x, y - 1, z + 1),
                               solid_array_isair(blocks, x - 1, y - 1, z + 1));

                tesselator_addi(tess, (int16_t[]) {x, y, z, x + 1, y, z, x + 1, y, z + 1, x, y, z + 1},
                                (RGBA4i[]) {
                                    {color.r * 0.5F * A, color.g * 0.5F * A, color.b * 0.5F * A, 255},
                                    {color.r * 0.5F * B, color.g * 0.5F * B, color.b * 0.5F * B, 255},
                                    {color.r * 0.5F * C, color.g * 0.5F * C, color.b * 0.5F * C, 255},
                                    {color.r * 0.5F * D, color.g * 0.5F * D, color.b * 0.5F * D, 255},
                                },
                                NULL);
            } else {
                tesselator_set_color(tess, (RGBA4i) {color.r * 0.5F, color.g * 0.5F, color.b * 0.5F, 255});
                tesselator_addi_cube_face(tess, CUBE_FACE_Y_N, x, y, z);
            }
        }
    }

    (*max_height)++;
}

void chunk_update_all(void) {
    size_t drain = channel_size(&chunk_result_queue);

    if (drain > 0) {
        ChunkResultPacket results[drain];

        for (size_t k = 0; k < drain; k++) {
            channel_await(&chunk_result_queue, results + k);
            results[k].chunk->updated = false;
        }

        ChunkResultPacket * result = results + drain - 1;

        for (size_t k = 0; k < drain; k++, result--) {
            if (!result->chunk->updated) {
                result->chunk->updated = true;

                if (!result->chunk->created) {
                    glx_displaylist_create(&result->chunk->display_list, true, false);
                    result->chunk->created = true;
                }

                result->chunk->max_height = result->max_height;

                tesselator_glx(&result->tesselator, &result->chunk->display_list);

                texture_subimage(
                    texture_minimap, result->chunk->x * CHUNK_SIZE, result->chunk->y * CHUNK_SIZE,
                    CHUNK_SIZE, CHUNK_SIZE, result->minimap_data
                );
            }

            tesselator_free(&result->tesselator);
            free(result->minimap_data);
        }
    }
}

void chunk_rebuild_all(void) {
    channel_clear(&chunk_work_queue);

    for (int k = CHUNKS_PER_DIM / 2; k >= 0; k--) {
        for (int i = k; i < CHUNKS_PER_DIM - k; i++) {
            Chunk * build[] = {
                chunks + i + k * CHUNKS_PER_DIM,
                chunks + i + (CHUNKS_PER_DIM - k - 1) * CHUNKS_PER_DIM,
                chunks + k + i * CHUNKS_PER_DIM,
                chunks + (CHUNKS_PER_DIM - k - 1) + i * CHUNKS_PER_DIM,
            };

            for (size_t j = 0; j < sizeof(build) / sizeof(*build); j++) {
                channel_put(&chunk_work_queue,
                            &(ChunkWorkPacket) {
                                .chunk   = build[j],
                                .chunk_x = build[j]->x,
                                .chunk_y = build[j]->y,
                            });
            }
        }
    }

    // Clean up “texture_minimap” as it can hold previous map or GPU garbage.
    texture_minimap_clear();
}

void chunk_block_update(int x, int y, int z) {
    UNUSED(y);

    Chunk * c = chunks + (x / CHUNK_SIZE) + (z / CHUNK_SIZE) * CHUNKS_PER_DIM;

    pthread_mutex_lock(&chunk_block_queue_lock);
    ht_insert(&chunk_block_queue, &c,
              &(ChunkWorkPacket) {
                  .chunk   = c,
                  .chunk_x = c->x,
                  .chunk_y = c->y,
              });
    pthread_mutex_unlock(&chunk_block_queue_lock);
}

static bool iterate_chunk_updates(void * key, void * value, void * user) {
    UNUSED(key);
    UNUSED(user);

    channel_put(&chunk_work_queue, value);
    return true;
}

void chunk_queue_blocks(void) {
    pthread_mutex_lock(&chunk_block_queue_lock);
    ht_iterate(&chunk_block_queue, NULL, iterate_chunk_updates);
    ht_clear(&chunk_block_queue);
    pthread_mutex_unlock(&chunk_block_queue_lock);
}
