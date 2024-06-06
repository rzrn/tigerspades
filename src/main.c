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

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <lodepng/lodepng.h>
#include <log.h>

#include <BetterSpades/common.h>
#include <BetterSpades/ping.h>
#include <BetterSpades/file.h>
#include <BetterSpades/font.h>
#include <BetterSpades/weapon.h>
#include <BetterSpades/window.h>
#include <BetterSpades/rpc.h>
#include <BetterSpades/network.h>
#include <BetterSpades/sound.h>
#include <BetterSpades/map.h>
#include <BetterSpades/particle.h>
#include <BetterSpades/tracer.h>
#include <BetterSpades/camera.h>
#include <BetterSpades/cameracontroller.h>
#include <BetterSpades/grenade.h>
#include <BetterSpades/player.h>
#include <BetterSpades/hud.h>
#include <BetterSpades/config.h>
#include <BetterSpades/matrix.h>
#include <BetterSpades/texture.h>
#include <BetterSpades/chunk.h>
#include <BetterSpades/unicode.h>
#include <BetterSpades/main.h>
#include <BetterSpades/opengl.h>

int fps = 0;

int ms_rand() {
    static int seed = 1;

    seed = seed * 0x343FD + 0x269EC3;
    return (seed >> 0x10) & 0x7FFF;
}

static mu_Rect window_clip = {.x = 0, .y = 0, .w = 0x1000000, .h = 0x1000000};

void window_scissor() {
    glScissor(window_clip.x, settings.window_height - window_clip.y - window_clip.h, window_clip.w, window_clip.h);
}

ChatInputMode chat_input_mode = CHAT_NO_INPUT;

char chat[2][10][256] = {{{0}}}; // chat[0] is current input

TrueColor chat_color[2][10];
float chat_timer[2][10];

void chat_add(int channel, TrueColor color, const char * msg, size_t size, Codepage codepage) {
    for (int k = 9; k > 1; k--) {
        strcpy(chat[channel][k], chat[channel][k - 1]);
        chat_color[channel][k] = chat_color[channel][k - 1];
        chat_timer[channel][k] = chat_timer[channel][k - 1];
    }

    convert(chat[channel][1], sizeof(chat[channel][1]), UTF8, msg, size, codepage);

    chat_color[channel][1] = color;
    chat_timer[channel][1] = window_time();

    if (channel == 0) log_info("%s", msg);
}

char chat_popup[256] = {0};
TrueColor chat_popup_color;
float chat_popup_timer = 0.0F;
float chat_popup_duration = 0.0F;

void chat_showpopup(const char * msg, size_t size, Codepage codepage, float duration, TrueColor color) {
    convert(chat_popup, sizeof(chat_popup), UTF8, msg, size, codepage);
    chat_popup_timer    = window_time();
    chat_popup_duration = duration;
    chat_popup_color    = color;
}

void drawScene() {
    if (settings.ambient_occlusion) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    matrix_upload();
    chunk_draw_visible();

    if (settings.smooth_fog) {
#ifdef OPENGL_ES
        glFogx(GL_FOG_MODE, GL_EXP2);
#else
        glFogi(GL_FOG_MODE, GL_EXP2);
#endif
        glFogf(GL_FOG_DENSITY, 0.015F);
        glFogfv(GL_FOG_COLOR, fog_color);
        glEnable(GL_FOG);
    }

    glShadeModel(GL_FLAT);
    kv6_calclight(-1, -1, -1);
    matrix_upload();
    trajectories_render_all();
    particle_render();
    tracer_render();
    grenade_render();
    map_damaged_voxels_render();
    matrix_upload();

    if (gamestate.mode == GAMEMODE_CTF) {
        if (!gamestate.ctf.team2_has_intel) {
            float x = gamestate.ctf.team1_flag.x;
            float y = gamestate.ctf.team1_flag.y + 1.0F;
            float z = gamestate.ctf.team1_flag.z;

            matrix_push(matrix_model);
            matrix_translate(matrix_model, x, y, z);
            kv6_calclight(x, y, z);
            matrix_upload();
            kv6_render(&model[MODEL_INTEL], TEAM1);
            matrix_pop(matrix_model);
        }

        if (!gamestate.ctf.team1_has_intel) {
            float x = gamestate.ctf.team2_flag.x;
            float y = gamestate.ctf.team2_flag.y + 1.0F;
            float z = gamestate.ctf.team2_flag.z;
            matrix_push(matrix_model);
            matrix_translate(matrix_model, x, y, z);
            kv6_calclight(x, y, z);
            matrix_upload();
            kv6_render(&model[MODEL_INTEL], TEAM2);
            matrix_pop(matrix_model);
        }

        if (map_object_visible(gamestate.ctf.team1_base.x, gamestate.ctf.team1_base.y + 1.0F, gamestate.ctf.team1_base.z)) {
            matrix_push(matrix_model);
            matrix_translate(matrix_model, gamestate.ctf.team1_base.x, gamestate.ctf.team1_base.y + 1.0F, gamestate.ctf.team1_base.z);
            kv6_calclight(gamestate.ctf.team1_base.x, gamestate.ctf.team1_base.y + 1.0F, gamestate.ctf.team1_base.z);
            matrix_upload();
            kv6_render(&model[MODEL_TENT], TEAM1);
            matrix_pop(matrix_model);
        }

        if (map_object_visible(gamestate.ctf.team2_base.x, gamestate.ctf.team2_base.y + 1.0F, gamestate.ctf.team2_base.z)) {
            matrix_push(matrix_model);
            matrix_translate(matrix_model, gamestate.ctf.team2_base.x, gamestate.ctf.team2_base.y + 1.0F, gamestate.ctf.team2_base.z);
            kv6_calclight(gamestate.ctf.team2_base.x, gamestate.ctf.team2_base.y + 1.0F, gamestate.ctf.team2_base.z);
            matrix_upload();
            kv6_render(&model[MODEL_TENT], TEAM2);
            matrix_pop(matrix_model);
        }
    }

    if (gamestate.mode == GAMEMODE_TC) {
        for (int k = 0; k < gamestate.tc.territory_count; k++) {
            matrix_push(matrix_model);
            matrix_translate(
                matrix_model,
                gamestate.tc.territory[k].pos.x,
                gamestate.tc.territory[k].pos.y + 1.0F,
                gamestate.tc.territory[k].pos.z
            );

            kv6_calclight(
                gamestate.tc.territory[k].pos.x,
                gamestate.tc.territory[k].pos.y + 1.0F,
                gamestate.tc.territory[k].pos.z
            );

            matrix_upload();
            kv6_render(&model[MODEL_TENT], min(gamestate.tc.territory[k].team, 2));
            matrix_pop(matrix_model);
        }
    }
}

void display() {
    if (hud_active->render_world)
        glClearColor(fog_color[0], fog_color[1], fog_color[2], fog_color[3]);
    else
        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (hud_active->render_world) {
        glEnable(GL_DEPTH_TEST);
        glDepthRange(0.0F, 1.0F);

        chunk_update_all();

        if (settings.opengl14) {
            matrix_identity(matrix_projection);
            matrix_perspective(matrix_projection, camera_fov_scaled(),
                               ((float) settings.window_width) / ((float) settings.window_height), 0.1F,
                               settings.render_distance + CHUNK_SIZE * 4.0F);
            matrix_upload_p();

            matrix_identity(matrix_view);
            camera_apply();
            matrix_identity(matrix_model);
            matrix_upload();

            float lpos[4] = {0.0F, -1.0F, 1.0F, 0.0F};
            glLightfv(GL_LIGHT0, GL_POSITION, lpos);
        }

        camera_ExtractFrustum();

        glx_enable_sphericalfog();
        drawScene();

        int render_fpv = (camera.mode == CAMERAMODE_FPS)
            || ((camera.mode == CAMERAMODE_BODYVIEW || camera.mode == CAMERAMODE_SPECTATOR)
                && cameracontroller_bodyview_mode);
        int is_local = (camera.mode == CAMERAMODE_FPS) || (cameracontroller_bodyview_player == local_player.id);
        int local_id = (camera.mode == CAMERAMODE_FPS) ? local_player.id : cameracontroller_bodyview_player;

        if (players[local_player.id].items_show && window_time() - players[local_player.id].items_show_start >= 0.5F)
            players[local_player.id].items_show = 0;

        if (camera.mode == CAMERAMODE_FPS) {
            weapon_update();

            if (HASBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY) &&
               (players[local_player.id].held_item == TOOL_BLOCK) &&
               (window_time() - players[local_player.id].item_showup >= 0.5F) &&
               (local_player.blocks > 0)) {
                int * pos = camera_terrain_pick(0);
                if (pos != NULL && isdestructible(pos[X], pos[Y], pos[Z])
                   && norm3f(camera.pos.x, camera.pos.y, camera.pos.z, pos[X], pos[Y], pos[Z]) < 25.0F
                   && !(pos[X] == (int) camera.pos.x && pos[Y] == (int) camera.pos.y + 0 && pos[Z] == (int) camera.pos.z)
                   && !(pos[X] == (int) camera.pos.x && pos[Y] == (int) camera.pos.y - 1 && pos[Z] == (int) camera.pos.z)) {
                    players[local_player.id].item_showup = window_time();

                    PacketBlockAction contained;
                    contained.player_id   = local_player.id;
                    contained.action_type = ACTION_BUILD;
                    contained.pos.x       = pos[X];
                    contained.pos.y       = pos[Z];
                    contained.pos.z       = 63 - pos[Y];

                    doPacketBlockAction(&contained);
                }
            }

            if (HASBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY) &&
               (players[local_player.id].held_item == TOOL_GRENADE) &&
               (window_time() - players[local_player.id].start.lmb > 3.0F)) {
                local_player.grenades = max(local_player.grenades - 1, 0);
                PacketGrenade contained;
                contained.player_id   = local_player.id;
                contained.pos         = htonv3f(players[local_player.id].pos);
                contained.vel         = (Vector3f) {0.0F, 0.0F, 0.0F};
                contained.fuse_length = 0.0F;

                sendPacketGrenade(&contained, 0);
                handlePacketGrenade(&contained); // see “src/hud.c”

                players[local_player.id].start.lmb = window_time();
            }
        }

        int * pos = NULL;
        switch (players[local_id].held_item) {
            case TOOL_BLOCK:
                if (!HASBIT(players[local_id].input.keys, INPUT_SPRINT) && render_fpv) {
                    if (is_local)
                        pos = camera_terrain_pick(0);
                    else
                        pos = camera_terrain_pickEx(
                            0, camera.pos.x, camera.pos.y, camera.pos.z, players[local_id].orientation_smooth.x,
                            players[local_id].orientation_smooth.y, players[local_id].orientation_smooth.z);
                }
                break;
            default: pos = NULL;
        }

        if (pos != NULL && isdestructible(pos[X], pos[Y], pos[Z]) &&
            norm3i(pos[X], pos[Y], pos[Z], camera.pos.x, camera.pos.y, camera.pos.z) < 25) {
            matrix_upload();
            glColor3f(1.0F, 0.0F, 0.0F);
            glLineWidth(1.0F);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            Vector3i cubes[64];
            int amount = 0;
            if (is_local && local_player.drag_active && HASBIT(players[local_player.id].input.buttons, BUTTON_SECONDARY)
               && players[local_player.id].held_item == TOOL_BLOCK) {
                amount = map_cube_line(local_player.drag.x, local_player.drag.z, 63 - local_player.drag.y,
                                       pos[0], pos[2], 63 - pos[1], cubes);
            } else {
                amount = 1;
                cubes[0].x = pos[0];
                cubes[0].y = pos[2];
                cubes[0].z = 63 - pos[1];
            }
            while (amount > 0) {
                int tmp = cubes[amount - 1].y;
                cubes[amount - 1].y = 63 - cubes[amount - 1].z;
                cubes[amount - 1].z = tmp;

                if (amount <= (is_local ? local_player.blocks : 50))
                    glColor3f(1.0F, 1.0F, 1.0F);

                short vertices[72] = {cubes[amount - 1].x,       cubes[amount - 1].y,        cubes[amount - 1].z,
                                      cubes[amount - 1].x,       cubes[amount - 1].y,        cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x,       cubes[amount - 1].y,        cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y,        cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y,        cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y,        cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y,        cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x,       cubes[amount - 1].y,        cubes[amount - 1].z + 1,

                                      cubes[amount - 1].x,       cubes[amount - 1].y + 1,    cubes[amount - 1].z,
                                      cubes[amount - 1].x,       cubes[amount - 1].y + 1,    cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x,       cubes[amount - 1].y + 1,    cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y + 1,    cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y + 1,    cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y + 1,    cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y + 1,    cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x,       cubes[amount - 1].y + 1,    cubes[amount - 1].z + 1,

                                      cubes[amount - 1].x,       cubes[amount - 1].y,        cubes[amount - 1].z,
                                      cubes[amount - 1].x,       cubes[amount - 1].y + 1,    cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y,        cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y + 1,    cubes[amount - 1].z,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y,        cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x + 1,   cubes[amount - 1].y + 1,    cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x,       cubes[amount - 1].y,        cubes[amount - 1].z + 1,
                                      cubes[amount - 1].x,       cubes[amount - 1].y + 1,    cubes[amount - 1].z + 1};
                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_SHORT, 0, vertices);
                glDrawArrays(GL_LINES, 0, 24);
                glDisableClientState(GL_VERTEX_ARRAY);
                amount--;
            }
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }

        if (window_time() - players[local_player.id].item_disabled < 0.3F) {
            players[local_player.id].item_showup = window_time();
            if (HASBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY))
                players[local_player.id].start.lmb = window_time() + 0.5F;
            if (HASBIT(players[local_player.id].input.buttons, BUTTON_SECONDARY))
                players[local_player.id].start.rmb = window_time() + 0.5F;
        } else {
            if (hud_active->render_localplayer) {
                float tmp2 = players[local_player.id].physics.eye.y;
                players[local_player.id].physics.eye.y = last_cy;
                if (camera.mode == CAMERAMODE_FPS)
                    glDepthRange(0.0F, 0.05F);
                matrix_push(matrix_projection);
                matrix_translate(matrix_projection, 0.0F, -0.25F, 0.0F);
                matrix_upload_p();
#ifdef OPENGL_ES
                if (camera.mode == CAMERAMODE_FPS)
                    glx_disable_sphericalfog();
#endif
                player_render(&players[local_player.id], local_player.id);
#ifdef OPENGL_ES
                if (camera.mode == CAMERAMODE_FPS)
                    glx_enable_sphericalfog();
#endif
                matrix_pop(matrix_projection);
                glDepthRange(0.0F, 1.0F);
                players[local_player.id].physics.eye.y = tmp2;
            }
        }

        matrix_upload_p();
        matrix_upload();
        player_render_all();

        matrix_upload();
        map_collapsing_render();
        matrix_upload();

#if !(HACKS_ENABLED && HACK_NOCLIP)
        if (!map_isair(camera.pos.x, camera.pos.y, camera.pos.z))
            glClear(GL_COLOR_BUFFER_BIT);
#endif

        glx_disable_sphericalfog();
        if (settings.smooth_fog)
            glDisable(GL_FOG);
    }

    if (hud_active->render_3D)
        hud_active->render_3D();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_MULTISAMPLE);
    matrix_identity(matrix_projection);
    matrix_ortho(matrix_projection, 0.0F, settings.window_width, 0.0F, settings.window_height, -1.0F, 1.0F);
    matrix_identity(matrix_view);
    matrix_identity(matrix_model);
    matrix_upload();
    matrix_upload_p();

    float scalex = fmax(1, round(settings.window_width / 800.0F));
    float scaley = fmax(1, round(settings.window_height / 600.0F));
    float scale  = settings.scale == 0 ? fmin(scalex, scaley) : settings.scale;

    if (hud_active->render_2D) {
        mu_Context * ctx = hud_active->ctx;

        if (ctx) {
            hud_active->ctx->style->padding        = 10 * scale - 5;
            hud_active->ctx->style->spacing        = 8  * scale - 4;
            hud_active->ctx->style->title_height   = 48 * scale - 24;
            hud_active->ctx->style->scrollbar_size = 12 * scale;
            hud_active->ctx->style->thumb_size     = 8  * scale;

            mu_begin(ctx);
        }

        hud_active->render_2D(ctx, scale);

        if (ctx) {
            mu_end(ctx);

            glEnable(GL_BLEND);
            glEnable(GL_SCISSOR_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            mu_Command * cmd = NULL;
            while (mu_next_command(ctx, &cmd)) {
                switch (cmd->type) {
                    case MU_COMMAND_TEXT:
                        glColor4ub(cmd->text.color.r, cmd->text.color.g, cmd->text.color.b, cmd->text.color.a);
                        font_render(cmd->text.pos.x, settings.window_height - cmd->text.pos.y,
                                    ctx->text_height(cmd->text.font) / 16.0F, cmd->text.str, UTF8);
                        glEnable(GL_BLEND);
                        break;
                    case MU_COMMAND_RECT:
                        glColor4ub(cmd->rect.color.r, cmd->rect.color.g, cmd->rect.color.b, cmd->rect.color.a);
                        texture_draw_empty(cmd->rect.rect.x, settings.window_height - cmd->rect.rect.y,
                                           cmd->rect.rect.w, cmd->rect.rect.h);
                        break;
                    case MU_COMMAND_ICON:
                        glColor4ub(cmd->icon.color.r, cmd->icon.color.g, cmd->icon.color.b, cmd->icon.color.a);
                        int size = min(cmd->icon.rect.w, cmd->icon.rect.h);

                        if (cmd->icon.id >= HUD_FLAG_INDEX_START - 1) {
                            float u, v;
                            texture_flag_offset(cmd->icon.id - HUD_FLAG_INDEX_START, &u, &v);
                            texture_draw_sector(
                                texture(TEXTURE_UI_FLAGS), cmd->icon.rect.x, settings.window_height - cmd->icon.rect.y,
                                size, size, u, v, 16.0F / 256.0F, 16.0F / 256.0F
                            );
                            glEnable(GL_BLEND);
                        } else if (hud_active->ui_images) {
                            bool resize = false;
                            Texture * img = hud_active->ui_images(cmd->icon.id, &resize);

                            if (img) {
                                texture_draw(img, cmd->icon.rect.x, settings.window_height - cmd->icon.rect.y,
                                             resize ? size : cmd->icon.rect.w, resize ? size : cmd->icon.rect.h);
                                glEnable(GL_BLEND);
                            }
                        }

                        break;
                    case MU_COMMAND_CLIP: {
                        window_clip = cmd->clip.rect;
                        window_scissor();
                        break;
                    }
                }
            }

            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_BLEND);
        }
    }

    if (settings.multisamples > 0)
        glEnable(GL_MULTISAMPLE);
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
#ifdef OPENGL_ES
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
#else
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
#endif
    glClearDepth(1.0F);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_FOG);

    map_init();

    glx_init();

    font_init();
    player_init();
    particle_init();
    network_init();
    ping_init();
    kv6_init();
    texture_init();
    sound_init();
    tracer_init();
    hud_init();
    chunk_init();
    grenade_init();

    weapon_set(false);

    rpc_init();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    settings.window_width  = width;
    settings.window_height = height;

    window_scissor();

    if (settings.vsync < 2)
        window_swapping(settings.vsync);

    if (settings.vsync > 1)
        window_swapping(0);
}

static int mu_button_translate(int button) {
    switch (button) {
        case WINDOW_MOUSE_LMB: return MU_MOUSE_LEFT;
        case WINDOW_MOUSE_MMB: return MU_MOUSE_MIDDLE;
        case WINDOW_MOUSE_RMB: return MU_MOUSE_RIGHT;
        default: return 0;
    }
}

static int mu_key_translate(int key) {
    switch (key) {
        case WINDOW_KEY_BACKSPACE: return MU_KEY_BACKSPACE;
        case WINDOW_KEY_ENTER:     return MU_KEY_RETURN;
        case WINDOW_KEY_SHIFT:     return MU_KEY_SHIFT;
        default: return 0;
    }
}

void text_input(const uint8_t * text) {
    const uint8_t * end = text + strlen((const char *) text);

    while (text != end) {
        char buff[5] = {0};

        size_t size = decodeSize(UTF8, text[0]);

        if (text[0] <= 0x7F && !isprint(text[0])) goto skip; // non-printable ASCII
        if (text[0] > 0xF7) goto skip; // invalid UTF-8

        // everything else assumed to be printable
        switch (size) {
            case 4: buff[3] = text[3];
            case 3: buff[2] = text[2];
            case 2: buff[1] = text[1];
            case 1: buff[0] = text[0];
        }

        if (hud_active->ctx) mu_input_text(hud_active->ctx, buff);

        if (chat_input_mode != CHAT_NO_INPUT) {
            size_t len = strlen(chat[0][0]);
            if (len + size < 128) strcpy(&chat[0][0][len], buff);
        }

        skip: text += size;
    }
}

void keys(int key, int action, int mods) {
    if (hud_active->ctx) {
        if (mu_key_translate(key)) {
            switch (action) {
                case WINDOW_RELEASE: mu_input_keyup(hud_active->ctx, mu_key_translate(key)); break;
                case WINDOW_REPEAT:
                case WINDOW_PRESS: mu_input_keydown(hud_active->ctx, mu_key_translate(key)); break;
            }
        }

        if (action == WINDOW_PRESS && key == WINDOW_KEY_V && mods) {
            const char * clipboard = window_clipboard();
            if (clipboard) mu_input_text(hud_active->ctx, clipboard);
        }
    }

    if (action == WINDOW_PRESS) {
        if (config_key(key)->toggle) {
            if (chat_input_mode == CHAT_NO_INPUT)
                window_pressed_keys[key] = !window_pressed_keys[key];
        } else window_pressed_keys[key] = 1;
    }

    if (action == WINDOW_RELEASE && !config_key(key)->toggle)
        window_pressed_keys[key] = 0;

    if (key == WINDOW_KEY_FULLSCREEN && action == WINDOW_PRESS) { // switch between fullscreen
        settings.fullscreen = !settings.fullscreen;
        window_videomode(settings.fullscreen);
    }

    if (key == WINDOW_KEY_SCREENSHOT && action == WINDOW_PRESS) { // take screenshot
        time_t pic_time;
        time(&pic_time);
        char pic_name[128];
        sprintf(pic_name, "screenshots/%ld.png", (long) pic_time);

        unsigned char * pic_data = malloc(settings.window_width * settings.window_height * 4 * 2);
        CHECK_ALLOCATION_ERROR(pic_data)
        glReadPixels(0, 0, settings.window_width, settings.window_height, GL_RGBA, GL_UNSIGNED_BYTE, pic_data);

        for (int y = 0; y < settings.window_height; y++) { // mirror image (top-bottom)
            for (int x = 0; x < settings.window_width; x++)
                pic_data[(x + (settings.window_height - y - 1) * settings.window_width) * 4 + 3] = 255;

            memcpy(
                pic_data + settings.window_width * 4 * (y + settings.window_height),
                pic_data + settings.window_width * 4 * (settings.window_height - y - 1),
                settings.window_width * 4
            );
        }

        lodepng_encode32_file(pic_name, pic_data + settings.window_width * settings.window_height * 4,
                              settings.window_width, settings.window_height);
        free(pic_data);

        sprintf(pic_name, "Saved screenshot as screenshots/%ld.png", (long) pic_time);
        chat_add(0, Red, pic_name, sizeof(pic_name), UTF8);
    }

    if (key == WINDOW_KEY_SAVE_MAP && action == WINDOW_PRESS) { // save map
        time_t save_time;
        time(&save_time);
        char save_name[128];
        sprintf(save_name, "vxl/%ld.vxl", (long) save_time);

        map_save_file(save_name);

        sprintf(save_name, "Saved map as vxl/%ld.vxl", (long) save_time);
        chat_add(0, Red, save_name, sizeof(save_name), UTF8);
    }
}

void mouse_click(int button, int action, int mods) {
    if (hud_active->input_mouseclick) {
        double x, y;
        window_mouseloc(&x, &y);
        hud_active->input_mouseclick(x, y, button, action, mods);
    }

    if (hud_active->ctx) {
        double x, y;
        window_mouseloc(&x, &y);
        switch (action) {
            case WINDOW_PRESS: mu_input_mousedown(hud_active->ctx, x, y, mu_button_translate(button)); break;
            case WINDOW_RELEASE: mu_input_mouseup(hud_active->ctx, x, y, mu_button_translate(button)); break;
        }
    }
}

void mouse_focus(bool focused) {
    if (hud_active != NULL && hud_active->focus) hud_active->focus(focused);
}

void mouse_hover(bool hovered) {
    if (hud_active != NULL && hud_active->hover) hud_active->hover(hovered);
}

void mouse(double x, double y) {
    if (hud_active->input_mouselocation) hud_active->input_mouselocation(x, y);
    if (hud_active->ctx) mu_input_mousemove(hud_active->ctx, x, y);
}

void mouse_scroll(double xoffset, double yoffset) {
    if (hud_active->input_mousescroll)
        hud_active->input_mousescroll(yoffset);

    if (hud_active->ctx)
        mu_input_scroll(hud_active->ctx, -xoffset * 50, -yoffset * 50);
}

void deinit() {
    rpc_deinit();
    ping_deinit();

    if (network_connected)
        network_disconnect();

    window_deinit();
    sound_deinit();
}

void on_error(int i, const char * s) {
    log_fatal("Major error occured: [%i] %s", i, s);
    getchar();
}

void idle(double dt) {
    static double physics_time_fixed = 0.0F;
    static double physics_time_fast  = 0.0F;

    if (hud_active->render_world) {
        physics_time_fast  += dt;
        physics_time_fixed += dt;

        // these run at exactly ~60fps
        #define PHYSICS_STEP_TIME (1.0 / 60.0)
        while (physics_time_fixed >= PHYSICS_STEP_TIME) {
            physics_time_fixed -= PHYSICS_STEP_TIME;
            player_update(PHYSICS_STEP_TIME, 1); // just physics tick
            grenade_update(PHYSICS_STEP_TIME);
        }

        // these run at min. ~60fps but as fast as possible
        double step = fmin(dt, PHYSICS_STEP_TIME);
        while (step > 0 && physics_time_fast >= step) {
            physics_time_fast -= step;
            player_update(step, 0); // smooth orientation update
            camera_update(step);
            tracer_update(step);
            particle_update(step);
            map_collapsing_update(step);
        }
    }

    sound_update();
    network_update();
    rpc_update();
}

static inline bool startswith(const char * prefix, const char * str)
{ return strncmp(prefix, str, strlen(prefix)) == 0; }

#define MATCH(x, y) if (!strcmp((x), (y)))
#define ERROR(retcode, ...) { printf(__VA_ARGS__); return retcode; }

int main(int argc, char ** argv) {
    const char * default_server = NULL;

    for (int i = 1; i < argc; i++) {
        if (startswith("-aos://", argv[i])) {
            default_server = argv[i] + 1;
        } else MATCH(argv[i], "--help") {
            ERROR(0, "Usage: %s -aos://<ip>:<port> --config <file> --team <team> --weapon <weapon> --serverlist <url> --newslist <url>\n", argv[0]);
        } else MATCH(argv[i], "--serverlist") {
            if (argc <= ++i) ERROR(-1, "The “--serverlist” option requires an argument.\n")
            else strnzcpy(serverlist_url, argv[i], sizeof(serverlist_url));
        } else MATCH(argv[i], "--newslist") {
            if (argc <= ++i) ERROR(-1, "The “--newslist” option requires an argument.\n")
            else strnzcpy(newslist_url, argv[i], sizeof(newslist_url));
        } else MATCH(argv[i], "--team") {
            if (argc <= ++i) ERROR(-1, "The “--team” option requires an argument.\n")
            else MATCH(argv[i], "1") default_team = TEAM1;
            else MATCH(argv[i], "2") default_team = TEAM2;
            else MATCH(argv[i], "3") default_team = TEAM_SPECTATOR;
            else ERROR(-2, "Unknown team (expected 1, 2, or 3).\n");
        } else MATCH(argv[i], "--weapon") {
            if (argc <= ++i) ERROR(-1, "The “--weapon” option requires an argument.\n")
            else MATCH(argv[i], "rifle")   default_gun = WEAPON_RIFLE;
            else MATCH(argv[i], "smg")     default_gun = WEAPON_SMG;
            else MATCH(argv[i], "shotgun") default_gun = WEAPON_SHOTGUN;
            else ERROR(-2, "Unknown weapon name (expected rifle, smg, or shotgun).\n");
        } else MATCH(argv[i], "--config") {
            if (argc <= ++i) ERROR(-1, "The “--config” option requires an argument.\n")
            else config_filepath = argv[i];
        } else MATCH(argv[i], "--offline") {
            offline = true;
        } else {
            ERROR(-3, "Unknown option “%s”.\n", argv[i]);
        }
    }

#ifdef USE_TOUCH
    mkdir("/sdcard/BetterSpades");
#else
    if (!file_dir_exists("logs"))
        file_dir_create("logs");
    if (!file_dir_exists("cache"))
        file_dir_create("cache");
    if (!file_dir_exists("screenshots"))
        file_dir_create("screenshots");
    if (!file_dir_exists("vxl"))
        file_dir_create("vxl");
#endif

    log_set_level(LOG_INFO);

    time_t t = time(NULL);
    char buf[32];
    strftime(buf, 32, "logs/%m-%d-%Y.log", localtime(&t));
    log_set_fp(fopen(buf, "a"));

    srand(t);

    log_info("TigerSpades " BETTERSPADES_VERSION);

    config_reload();

    window_init(&argc, argv);

#ifndef OPENGL_ES
    if (glewInit())
        log_error("Could not load extended OpenGL functions!");
#endif

    log_info("Vendor: %s",   glGetString(GL_VENDOR));
    log_info("Renderer: %s", glGetString(GL_RENDERER));
    log_info("Version: %s",  glGetString(GL_VERSION));

    if (settings.multisamples > 0) {
        glEnable(GL_MULTISAMPLE);
        log_info("MSAAx%i on", settings.multisamples);
    }

    while (glGetError() != GL_NO_ERROR);

    init(); atexit(deinit);

    if (settings.vsync < 2)
        window_swapping(settings.vsync);

    if (settings.vsync > 1)
        window_swapping(0);

    if (default_server != NULL) {
        if (!network_connect_string(default_server, VER07X)) {
            log_error("Error: Connection failed (use --help for instructions)");
            exit(1);
        } else {
            log_info("Connection to %s successful", default_server);
            hud_change(&hud_mapload);
        }
    }

    window_eventloop(idle, display);
}
