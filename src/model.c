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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <BetterSpades/common.h>
#include <BetterSpades/player.h>
#include <BetterSpades/file.h>
#include <BetterSpades/camera.h>
#include <BetterSpades/matrix.h>
#include <BetterSpades/map.h>
#include <BetterSpades/config.h>
#include <BetterSpades/model.h>
#include <BetterSpades/model_normals.h>
#include <BetterSpades/texture.h>
#include <BetterSpades/opengl.h>

#include <log.h>

kv6 model[MODEL_TOTAL];

typedef struct {
    const char * filename;
    float scale, max_size;
} Resource;

static Resource kv6_model(enum kv6 index) {
    switch (index) {
        case MODEL_PLAYERDEAD:     return (Resource) {"kv6/playerdead.kv6",    0.1000F, INFINITY};
        case MODEL_PLAYERHEAD:     return (Resource) {"kv6/playerhead.kv6",    0.1000F, 1.2F};
        case MODEL_PLAYERTORSO:    return (Resource) {"kv6/playertorso.kv6",   0.1000F, 1.8F};
        case MODEL_PLAYERTORSOC:   return (Resource) {"kv6/playertorsoc.kv6",  0.1000F, 1.6F};
        case MODEL_PLAYERARMS:     return (Resource) {"kv6/playerarms.kv6",    0.1000F, 2.0F};
        case MODEL_PLAYERLEG:      return (Resource) {"kv6/playerleg.kv6",     0.1000F, 2.0F};
        case MODEL_PLAYERLEGC:     return (Resource) {"kv6/playerlegc.kv6",    0.1000F, 1.6F};

        case MODEL_INTEL:          return (Resource) {"kv6/intel.kv6",         0.2000F, INFINITY};
        case MODEL_TENT:           return (Resource) {"kv6/cp.kv6",            0.2780F, INFINITY};

        case MODEL_SEMI:           return (Resource) {"kv6/semi.kv6",          0.0500F, 2.25F};
        case MODEL_SMG:            return (Resource) {"kv6/smg.kv6",           0.0500F, 2.25F};
        case MODEL_SHOTGUN:        return (Resource) {"kv6/shotgun.kv6",       0.0500F, 2.25F};
        case MODEL_SPADE:          return (Resource) {"kv6/spade.kv6",         0.0500F, 2.25F};
        case MODEL_BLOCK:          return (Resource) {"kv6/block.kv6",         0.0500F, 2.25F};
        case MODEL_GRENADE:        return (Resource) {"kv6/grenade.kv6",       0.0500F, 2.25F};

        case MODEL_SEMI_TRACER:    return (Resource) {"kv6/semitracer.kv6",    0.0500F, INFINITY};
        case MODEL_SMG_TRACER:     return (Resource) {"kv6/smgtracer.kv6",     0.0500F, INFINITY};
        case MODEL_SHOTGUN_TRACER: return (Resource) {"kv6/shotguntracer.kv6", 0.0500F, INFINITY};

        case MODEL_SEMI_CASING:    return (Resource) {"kv6/semicasing.kv6",    0.0125F, INFINITY};
        case MODEL_SMG_CASING:     return (Resource) {"kv6/smgcasing.kv6",     0.0125F, INFINITY};
        case MODEL_SHOTGUN_CASING: return (Resource) {"kv6/shotguncasing.kv6", 0.0125F, INFINITY};
    }

    return (Resource) {NULL, 0.0F, 0.0F};
}

static void kv6_load_file(kv6 * model, const char * filename, float scale) {
    uint8_t * data = file_load(filename);
    kv6_load(model, filename, data, scale);
    free(data);
}

static void kv6_check_dimensions(kv6 * model, const char * name, float max) {
    if (max(max(model->xsiz, model->ysiz), model->zsiz) * model->scale > max) {
        log_error("%s: model dimensions too large", name);
        model->voxel_count = 0;
    }
}

void kv6_init() {
    for (enum kv6 i = MODEL_FIRST; i <= MODEL_LAST; i++) {
        Resource res = kv6_model(i);
        kv6_load_file(&model[i], res.filename, res.scale);
        kv6_check_dimensions(&model[i], res.filename, res.max_size);
    }

    model[MODEL_BLOCK].colorize = true;
}

void kv6_rebuild_complete() {
    for (enum kv6 i = MODEL_FIRST; i <= MODEL_LAST; i++)
        kv6_rebuild(&model[i]);
}

void kv6_load(kv6 * model, const char * name, uint8_t * buff, float scale) {
    // https://gist.github.com/falkreon/8b873ec6797ffad247375fc73614fd08

    model->colorize         = false;
    model->has_display_list = false;
    model->scale            = scale;

    size_t index = 0;

    if (getu32le(buff, &index) == 0x6C78764B) { // Kvxl
        model->xsiz = getu32le(buff, &index);
        model->ysiz = getu32le(buff, &index);
        model->zsiz = getu32le(buff, &index);

        model->xpiv = getf32le(buff, &index);
        model->ypiv = getf32le(buff, &index);
        model->zpiv = model->zsiz - getf32le(buff, &index);

        model->voxel_count = getu32le(buff, &index);

        model->voxels = malloc(sizeof(Voxel) * model->voxel_count);
        CHECK_ALLOCATION_ERROR(model->voxels)

        for (size_t k = 0; k < model->voxel_count; k++) {
            model->voxels[k].color    = getbgra(buff, &index); // for whatever reason .kv6 seems to be stored in BGRA
            model->voxels[k].z        = (model->zsiz - 1) - getu16le(buff, &index);
            model->voxels[k].visfaces = getu8le(buff, &index); // 0x00zZyYxX
            model->voxels[k].color.a  = getu8le(buff, &index); // compressed normal vector (also referred to as lighting)
        }

        index += sizeof(uint32_t) * model->xsiz;

        Voxel * voxel = model->voxels;
        for (size_t x = 0; x < model->xsiz; x++) {
            for (size_t y = 0; y < model->ysiz; y++) {
                uint16_t size = getu16le(buff, &index);

                for (size_t z = 0; z < size; z++, voxel++) {
                    voxel->x = x;
                    voxel->y = y;
                }
            }
        }
    } else {
        log_error("%s: data not in kv6 format", name);
        model->xsiz = model->ysiz = model->zsiz = 0;
        model->voxel_count = 0;
    }
}

void kv6_rebuild(kv6 * model) {
    if (model->has_display_list) {
        glx_displaylist_destroy(model->display_list + 0);
        glx_displaylist_destroy(model->display_list + 1);
        model->has_display_list = false;
    }
}

void kv6_calclight(int x, int y, int z) {
    float f = 1.0F;

    if (x >= 0 && y >= 0 && z >= 0)
        f = map_sunblock(x, y, z);

    float lambient[4] = {0.5F * f, 0.5F * f, 0.5F * f, 1.0F};
    float ldiffuse[4] = {0.5F * f, 0.5F * f, 0.5F * f, 1.0F};

    glLightfv(GL_LIGHT0, GL_AMBIENT, lambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, ldiffuse);
}

static int kv6_voxel_cmp(const void * a, const void * b) {
    const Voxel * A = a;
    const Voxel * B = b;

    if (A->x == B->x) {
        if (A->y == B->y) {
            return B->z - A->z;
        } else {
            return A->y - B->y;
        }
    } else {
        return A->x - B->x;
    }
}

static void greedy_mesh(kv6 * model, Voxel * voxel, uint8_t * marked, size_t * max_a, size_t * max_b, uint8_t face) {
    if (!settings.greedy_meshing) {
        *max_a = 1;
        *max_b = 1;
    } else {
        switch (face) {
            case KV6_VIS_POS_X:
            case KV6_VIS_NEG_X:
                *max_a = model->ysiz;
                *max_b = model->zsiz;
                break;
            case KV6_VIS_POS_Y:
            case KV6_VIS_NEG_Y:
                *max_a = model->xsiz;
                *max_b = model->ysiz;
                break;
            case KV6_VIS_POS_Z:
            case KV6_VIS_NEG_Z:
                *max_a = model->xsiz;
                *max_b = model->zsiz;
                break;
        }

        Voxel lookup = *voxel;

        for (size_t a = 0; a < *max_a; a++) {
            Voxel * recent[*max_b];
            size_t b;
            for (b = 0; b < *max_b; b++) {
                switch (face) {
                    case KV6_VIS_POS_X:
                    case KV6_VIS_NEG_X:
                        lookup.y = voxel->y + b;
                        lookup.z = voxel->z - a;
                        break;
                    case KV6_VIS_POS_Y:
                    case KV6_VIS_NEG_Y:
                        lookup.x = voxel->x + a;
                        lookup.y = voxel->y + b;
                        break;
                    case KV6_VIS_POS_Z:
                    case KV6_VIS_NEG_Z:
                        lookup.x = voxel->x + a;
                        lookup.z = voxel->z - b;
                        break;
                }

                Voxel * neighbour = bsearch(&lookup, model->voxels, model->voxel_count, sizeof(Voxel), kv6_voxel_cmp);

                if (!neighbour || !(neighbour->visfaces & face)
                   || neighbour->color.r != voxel->color.r
                   || neighbour->color.g != voxel->color.g
                   || neighbour->color.b != voxel->color.b
                   || marked[neighbour - model->voxels] & face) {
                    break;
                } else {
                    marked[neighbour - model->voxels] |= face;
                    recent[b] = neighbour;
                }
            }

            if (a == 0)
                *max_b = b;

            if (b < *max_b) { // early abort
                *max_a = a;

                for (size_t k = 0; k < b; k++)
                    marked[recent[k] - model->voxels] &= ~face;

                break;
            }
        }
    }
}

static int kv6_program = -1;
void kv6_render(kv6 * model, unsigned char team) {
    if (!model)
        return;
    if (team == TEAM_SPECTATOR)
        team = 2;
    if (!settings.voxlap_models) {
        if (!model->has_display_list) {
            Tesselator tess_color; tesselator_create(&tess_color, VERTEX_INT, 1);
            Tesselator tess_team;  tesselator_create(&tess_team, VERTEX_INT, 1);

            glx_displaylist_create(model->display_list + 0, true, true);
            glx_displaylist_create(model->display_list + 1, true, true);

            uint8_t marked[model->voxel_count];
            memset(marked, 0, sizeof(uint8_t) * model->voxel_count);

            Voxel * voxel = model->voxels;
            for (size_t k = 0; k < model->voxel_count; k++, voxel++) {
                int r = voxel->color.r;
                int g = voxel->color.g;
                int b = voxel->color.b;
                int a = voxel->color.a;

                Tesselator * tess = &tess_color;

                if ((r | g | b) == 0) {
                    tess = &tess_team;
                    r = g = b = 255;
                } else if (model->colorize) {
                    r = g = b = 255;
                }

                tesselator_set_normal(tess, kv6_normals[a][0] * 128, -kv6_normals[a][2] * 128, kv6_normals[a][1] * 128);

                if (voxel->visfaces & KV6_VIS_POS_Y) {
                    size_t max_x, max_z;
                    greedy_mesh(model, voxel, marked, &max_x, &max_z, KV6_VIS_POS_Y);

                    tesselator_set_color(tess, (TrueColor) {r, g, b, 0});
                    tesselator_addi_cube_face_adv(tess, CUBE_FACE_Y_P, voxel->x, voxel->z, voxel->y, max_x, 1, max_z);
                }

                if (voxel->visfaces & KV6_VIS_NEG_Y) {
                    size_t max_x, max_z;
                    greedy_mesh(model, voxel, marked, &max_x, &max_z, KV6_VIS_NEG_Y);

                    tesselator_set_color(tess, (TrueColor) {r * 0.6F, g * 0.6F, b * 0.6F, 0});
                    tesselator_addi_cube_face_adv(tess, CUBE_FACE_Y_N, voxel->x, voxel->z, voxel->y, max_x, 1, max_z);
                }

                if (voxel->visfaces & KV6_VIS_NEG_Z) {
                    size_t max_x, max_y;
                    greedy_mesh(model, voxel, marked, &max_x, &max_y, KV6_VIS_NEG_Z);

                    tesselator_set_color(tess, (TrueColor) {r * 0.95F, g * 0.95F, b * 0.95F, 0});
                    tesselator_addi_cube_face_adv(tess, CUBE_FACE_Z_N, voxel->x, voxel->z - (max_y - 1), voxel->y,
                                                  max_x, max_y, 1);
                }

                if (voxel->visfaces & KV6_VIS_POS_Z) {
                    size_t max_x, max_y;
                    greedy_mesh(model, voxel, marked, &max_x, &max_y, KV6_VIS_POS_Z);

                    tesselator_set_color(tess, (TrueColor) {r * 0.9F, g * 0.9F, b * 0.9F, 0});
                    tesselator_addi_cube_face_adv(tess, CUBE_FACE_Z_P, voxel->x, voxel->z - (max_y - 1), voxel->y,
                                                  max_x, max_y, 1);
                }

                if (voxel->visfaces & KV6_VIS_NEG_X) {
                    size_t max_y, max_z;
                    greedy_mesh(model, voxel, marked, &max_y, &max_z, KV6_VIS_NEG_X);

                    tesselator_set_color(tess, (TrueColor) {r * 0.85F, g * 0.85F, b * 0.85F, 0});
                    tesselator_addi_cube_face_adv(tess, CUBE_FACE_X_N, voxel->x, voxel->z - (max_y - 1), voxel->y, 1,
                                                  max_y, max_z);
                }

                if (voxel->visfaces & KV6_VIS_POS_X) {
                    size_t max_y, max_z;
                    greedy_mesh(model, voxel, marked, &max_y, &max_z, KV6_VIS_POS_X);

                    tesselator_set_color(tess, (TrueColor) {r * 0.8F, g * 0.8F, b * 0.8F, 0});
                    tesselator_addi_cube_face_adv(tess, CUBE_FACE_X_P, voxel->x, voxel->z - (max_y - 1), voxel->y, 1,
                                                  max_y, max_z);
                }
            }

            tesselator_glx(&tess_color, model->display_list + 0);
            tesselator_glx(&tess_team, model->display_list + 1);

            tesselator_free(&tess_color);
            tesselator_free(&tess_team);

            model->has_display_list = true;
        } else {
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
            glEnable(GL_COLOR_MATERIAL);
#ifndef OPENGL_ES
            glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
#endif
            glEnable(GL_NORMALIZE);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_CONSTANT);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_CONSTANT);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PREVIOUS);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
            texture_bind(texture_dummy);

            if (model->colorize) {
                glEnable(GL_TEXTURE_2D);
                glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (float[]) {model->red, model->green, model->blue, 1.0F});
            }

            matrix_push(matrix_model);
            matrix_scale3(matrix_model, model->scale);
            matrix_translate(matrix_model, -model->xpiv, -model->zpiv, -model->ypiv);
            matrix_upload();

            glx_displaylist_draw(model->display_list + 0, GLX_DISPLAYLIST_NORMAL);

            if (!model->colorize)
                glEnable(GL_TEXTURE_2D);

            switch (team) {
                case TEAM_1: {
                    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,
                               (float[]) {gamestate.team1.color.r * 0.75F / 255.0F,
                                          gamestate.team1.color.g * 0.75F / 255.0F,
                                          gamestate.team1.color.b * 0.75F / 255.0F,
                                          1.0F});
                    break;
                }

                case TEAM_2: {
                    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,
                               (float[]) {gamestate.team2.color.r * 0.75F / 255.0F,
                                          gamestate.team2.color.g * 0.75F / 255.0F,
                                          gamestate.team2.color.b * 0.75F / 255.0F,
                                          1.0F});
                    break;
                }

                default: glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (float[]) {0, 0, 0, 1});
            }

            glx_displaylist_draw(model->display_list + 1, GLX_DISPLAYLIST_NORMAL);

            matrix_pop(matrix_model);

            glBindTexture(GL_TEXTURE_2D, 0);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_TEXTURE_2D);

            glDisable(GL_NORMALIZE);
            glDisable(GL_COLOR_MATERIAL);
            glDisable(GL_LIGHT0);
            glDisable(GL_LIGHTING);
        }
    } else {
        // render like on voxlap
        if (!model->has_display_list) {
            float vertices[2][model->voxel_count * 3];
            uint8_t colors[2][model->voxel_count * 4];
            int8_t normals[2][model->voxel_count * 3];
            model->has_display_list = true;

            int cnt[2] = {0, 0};

            glx_displaylist_create(model->display_list + 0, !model->colorize, true);
            glx_displaylist_create(model->display_list + 1, false, true);

            for (size_t i = 0; i < model->voxel_count; i++) {
                int r = model->voxels[i].color.r;
                int g = model->voxels[i].color.g;
                int b = model->voxels[i].color.b;
                int a = model->voxels[i].color.a;

                int ind = ((r | g | b) == 0) ? 1 : 0;

                colors[ind][cnt[ind] * 4 + 0] = r;
                colors[ind][cnt[ind] * 4 + 1] = g;
                colors[ind][cnt[ind] * 4 + 2] = b;
                colors[ind][cnt[ind] * 4 + 3] = 255;

                normals[ind][cnt[ind] * 3 + 0] = kv6_normals[a][0] * 128;
                normals[ind][cnt[ind] * 3 + 1] = -kv6_normals[a][2] * 128;
                normals[ind][cnt[ind] * 3 + 2] = kv6_normals[a][1] * 128;

                vertices[ind][cnt[ind] * 3 + 0] = (model->voxels[i].x - model->xpiv + 0.5F) * model->scale;
                vertices[ind][cnt[ind] * 3 + 1] = (model->voxels[i].z - model->zpiv + 0.5F) * model->scale;
                vertices[ind][cnt[ind] * 3 + 2] = (model->voxels[i].y - model->ypiv + 0.5F) * model->scale;

                cnt[ind]++;
            }

            glx_displaylist_update(model->display_list + 0, cnt[0], GLX_DISPLAYLIST_POINTS, colors[0], vertices[0],
                                   normals[0]);

            glx_displaylist_update(model->display_list + 1, cnt[1], GLX_DISPLAYLIST_POINTS, colors[1], vertices[1],
                                   normals[1]);

            if (kv6_program < 0) {
                kv6_program
                    = glx_shader("uniform float size;\n"
                                 "uniform vec3 fog;\n"
                                 "uniform vec3 camera;\n"
                                 "uniform mat4 model;\n"
                                 "uniform float dist_factor;\n"
                                 "void main(void) {\n"
                                 "    gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;\n"
                                 "    float dist = length((model*gl_Vertex).xz-camera.xz)*dist_factor;\n"
                                 "    vec3 N = normalize(model*vec4(gl_Normal,0)).xyz;\n"
                                 "    vec3 L = normalize(vec3(0,-1,1));\n"
                                 "    float d = clamp(dot(N,L),0.0,1.0)*0.5+0.5;\n"
                                 "    gl_FrontColor = mix(vec4(d,d,d,1.0)*gl_Color,vec4(fog,1.0),min(dist,1.0));\n"
                                 "    gl_PointSize = size/gl_Position.w;\n"
                                 "}\n",
                                 "void main(void) {\n"
                                 "    gl_FragColor = gl_Color;\n"
                                 "}\n");
            }
        }

        float near_plane_height
            = (float) settings.window_height / (2.0F * tan(glm_persp_fovy(matrix_projection) / 2.0F));

        float len_x = hypot3f(matrix_model[0][0], matrix_model[1][0], matrix_model[2][0]);
        float len_y = hypot3f(matrix_model[0][1], matrix_model[1][1], matrix_model[2][1]);
        float len_z = hypot3f(matrix_model[0][2], matrix_model[1][2], matrix_model[2][2]);

#ifndef OPENGL_ES
        if (!glx_version)
#endif
        {
            glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, (float[]) {0.0F, 0.0F, 1.0F});
            glPointSize(1.414F * near_plane_height * model->scale * (len_x + len_y + len_z) / 3.0F);
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
            glEnable(GL_COLOR_MATERIAL);
#ifndef OPENGL_ES
            glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
#endif
            glEnable(GL_NORMALIZE);
        }

#ifndef OPENGL_ES
        if (glx_version) {
            glEnable(GL_PROGRAM_POINT_SIZE);
            glUseProgram(kv6_program);
            glUniform1f(glGetUniformLocation(kv6_program, "dist_factor"),
                        glx_fog ? 1.0F / settings.render_distance : 0.0F);
            glUniform1f(glGetUniformLocation(kv6_program, "size"),
                        1.414F * near_plane_height * model->scale * (len_x + len_y + len_z) / 3.0F);
            glUniform3f(glGetUniformLocation(kv6_program, "fog"), fog_color[0], fog_color[1], fog_color[2]);
            glUniform3f(glGetUniformLocation(kv6_program, "camera"), camera.pos.x, camera.pos.y, camera.pos.z);
            glUniformMatrix4fv(glGetUniformLocation(kv6_program, "model"), 1, 0, (float*) matrix_model);
        }
#endif
        if (settings.multisamples)
            glDisable(GL_MULTISAMPLE);

        if (model->colorize)
            glColor3f(model->red, model->green, model->blue);

        glx_displaylist_draw(model->display_list + 0, GLX_DISPLAYLIST_POINTS);

        switch (team) {
            case TEAM_1: glColorRGB3ib(gamestate.team1.color, 0.75F); break;
            case TEAM_2: glColorRGB3ib(gamestate.team2.color, 0.75F); break;
            default: glColor3ub(0, 0, 0);
        }

        glx_displaylist_draw(model->display_list + 1, GLX_DISPLAYLIST_POINTS);

        if (settings.multisamples)
            glEnable(GL_MULTISAMPLE);
#ifndef OPENGL_ES
        if (glx_version) {
            glUseProgram(0);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
#endif

#ifndef OPENGL_ES
        if (!glx_version)
#endif
        {
            glDisable(GL_NORMALIZE);
            glDisable(GL_COLOR_MATERIAL);
            glDisable(GL_LIGHT0);
            glDisable(GL_LIGHTING);
        }
    }
}
