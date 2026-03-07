/*
    Copyright © 2016–2023 ByteBit
    Copyright © 2018 feikname
    Copyright © 2018 vuolen
    Copyright © 2018 yvt
    Copyright © 2022 Haxk20
    Copyright © 2025 Ashy
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

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <lodepng/lodepng.h>
#include <log.h>

#include <bs/common.h>
#include <bs/ping.h>
#include <bs/file.h>
#include <bs/font.h>
#include <bs/weapon.h>
#include <bs/window.h>
#include <bs/rpc.h>
#include <bs/network.h>
#include <bs/sound.h>
#include <bs/map.h>
#include <bs/particle.h>
#include <bs/tracer.h>
#include <bs/camera.h>
#include <bs/cameracontroller.h>
#include <bs/grenade.h>
#include <bs/player.h>
#include <bs/hud.h>
#include <bs/config.h>
#include <bs/matrix.h>
#include <bs/texture.h>
#include <bs/chunk.h>
#include <bs/unicode.h>
#include <bs/main.h>
#include <bs/opengl.h>

int fps = 0;

int ms_rand(void) {
    static int seed = 1;

    seed = seed * 0x343FD + 0x269EC3;
    return (seed >> 0x10) & 0x7FFF;
}

static mu_Rect window_clip = {.x = 0, .y = 0, .w = 0x1000000, .h = 0x1000000};

void window_scissor(void) {
    glScissor(window_clip.x, settings.window_height - window_clip.y - window_clip.h, window_clip.w, window_clip.h);
}

ChatInputMode chat_input_mode = CHAT_NO_INPUT;

TextDeque game_chat, game_killfeed = {.maxlen = 5};
char game_chat_input[CHAT_MESSAGE_SIZE];

static inline bool is_deque_full(TextDeque * deque)
{ return deque->last != NULL && deque->maxlen <= deque->first->index - deque->last->index; }

static inline void deque_push(TextDeque * deque, Text * node) {
    node->prev = NULL;
    node->next = deque->first;

    if (deque->first == NULL) {
        node->index = 0;
        deque->last = node;
    } else {
        node->index = deque->first->index + 1;
        deque->first->prev = node;
    }

    deque->first = node;
}

static inline Text * deque_pop(TextDeque * deque) {
    Text * retval = deque->last;

    if (retval->prev == NULL) {
        deque->first = NULL;
        deque->last = NULL;
    } else {
        deque->last = retval->prev;
        deque->last->next = NULL;
    }

    return retval;
}

void deque_free(TextDeque * deque) {
    for (Text * node = deque->first; node != NULL;) {
        Text * next = node->next;
        free(node); node = next;
    }

    deque->first = deque->last = NULL;
}

void game_killfeed_add(RGBA4i color, const char * mesg, size_t size) {
    // Instead of free-malloc, we reuse existing memory.
    Text * node = is_deque_full(&game_killfeed)
                ? deque_pop(&game_killfeed)
                : malloc(sizeof(Text));

    strncpy(node->value, mesg, sizeof(node->value));
    node->timer = window_time();
    node->color = color;

    deque_push(&game_killfeed, node);
}

void game_chat_add(RGBA4i color, const char * mesg, size_t size, Codepage codepage) {
    Text * node;

    if (is_deque_full(&game_chat)) {
        node = deque_pop(&game_chat);

        if (hud_game_chat_selected == node)
            hud_game_chat_selected = game_chat.last;
    } else {
        node = malloc(sizeof(Text));
    }

    convert(node->value, sizeof(node->value), UTF8, mesg, size, codepage);
    node->timer = window_time();
    node->color = color;

    deque_push(&game_chat, node);

    log_info("%s", mesg);
}

Text chat_popup; float chat_popup_duration = 0.0F;

void chat_show_popup(const char * msg, size_t size, Codepage codepage, float duration, RGBA4i color) {
    convert(chat_popup.value, sizeof(chat_popup.value), UTF8, msg, size, codepage);
    chat_popup.timer = window_time();
    chat_popup.color = color;

    chat_popup_duration = duration;
}

void drawEntity(kv6 * model, Vector3f * r, unsigned char team) {
    float x = r->x, y = r->y + 1.0F, z = r->z;

    if (!isfinite(x) || !isfinite(y) || !isfinite(z)) return;

    matrix_push(matrix_model);
    matrix_translate(matrix_model, x, y, z);
    kv6_calclight(x, y, z);
    matrix_upload();
    kv6_render(model, team);
    matrix_pop(matrix_model);
}

void drawScene(void) {
    glShadeModel(settings.ambient_occlusion ? GL_SMOOTH : GL_FLAT);

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
        if (!gamestate.ctf.team2_has_intel)
            drawEntity(&model[MODEL_INTEL], &gamestate.ctf.team1_flag, TEAM1);

        if (!gamestate.ctf.team1_has_intel)
            drawEntity(&model[MODEL_INTEL], &gamestate.ctf.team2_flag, TEAM2);

        if (map_object_visible(gamestate.ctf.team1_base.x, gamestate.ctf.team1_base.y + 1.0F, gamestate.ctf.team1_base.z))
            drawEntity(&model[MODEL_TENT], &gamestate.ctf.team1_base, TEAM1);

        if (map_object_visible(gamestate.ctf.team2_base.x, gamestate.ctf.team2_base.y + 1.0F, gamestate.ctf.team2_base.z))
            drawEntity(&model[MODEL_TENT], &gamestate.ctf.team2_base, TEAM2);
    }

    if (gamestate.mode == GAMEMODE_TC) {
        for (int k = 0; k < gamestate.tc.territory_count; k++)
            drawEntity(&model[MODEL_TENT], &gamestate.tc.territory[k].pos, gamestate.tc.territory[k].team);
    }
}

static inline void drawCubeEdges(int x, int y, int z) {
    short vertices[] = {
        x,       y,        z,
        x,       y,        z + 1,
        x,       y,        z,
        x + 1,   y,        z,
        x + 1,   y,        z + 1,
        x + 1,   y,        z,
        x + 1,   y,        z + 1,
        x,       y,        z + 1,

        x,       y + 1,    z,
        x,       y + 1,    z + 1,
        x,       y + 1,    z,
        x + 1,   y + 1,    z,
        x + 1,   y + 1,    z + 1,
        x + 1,   y + 1,    z,
        x + 1,   y + 1,    z + 1,
        x,       y + 1,    z + 1,

        x,       y,        z,
        x,       y + 1,    z,
        x + 1,   y,        z,
        x + 1,   y + 1,    z,
        x + 1,   y,        z + 1,
        x + 1,   y + 1,    z + 1,
        x,       y,        z + 1,
        x,       y + 1,    z + 1
    };

    glVertexPointer(3, GL_SHORT, 0, vertices);
    glDrawArrays(GL_LINES, 0, lengthof(vertices) / 3);
}

void game_display(void) {
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
            matrix_perspective(
                matrix_projection, camera_fov_scaled(), window_aspect(), frustum_near(), frustum_far()
            );
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
            players[local_player.id].items_show = false;

        if (camera.mode == CAMERAMODE_FPS) {
            weapon_update();

            if (!network_connected && button_map.mmb &&
                players[local_player.id].tool == TOOL_BLOCK &&
                window_time() - players[local_player.id].item_showup >= 0.5F) {
                int * pick = camera_terrain_pick(1);

                if (pick != NULL) {
                    players[local_player.id].item_showup = window_time();

                    RGBA4i color = RGB3iAs4i(players[local_player.id].block);
                    map_set(pick[X], pick[Y], pick[Z], &color);
                }
            }

            if (HASBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY) &&
                players[local_player.id].tool == TOOL_BLOCK &&
                window_time() - players[local_player.id].item_showup >= 0.5F &&
                local_player.blocks > 0) {
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

                    SETBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY, false);
                }
            }

            if (HASBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY) &&
                players[local_player.id].tool == TOOL_GRENADE &&
                window_time() - players[local_player.id].start.lmb > 3.0F) {
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
        switch (players[local_id].tool) {
            case TOOL_BLOCK:
                if (!HASBIT(players[local_id].input.keys, INPUT_SPRINT) && render_fpv) {
                    if (is_local)
                        pos = camera_terrain_pick(0);
                    else
                        pos = camera_terrain_pickEx(
                            0, camera.pos.x, camera.pos.y, camera.pos.z,
                            players[local_id].orientation_smooth.x,
                            players[local_id].orientation_smooth.y,
                            players[local_id].orientation_smooth.z
                        );
                }

                break;

            default: pos = NULL;
        }

        if (players[local_id].alive && players[local_id].tool == TOOL_BLOCK)
        if (pos != NULL && norm3f(pos[X], pos[Y], pos[Z], camera.pos.x, camera.pos.y, camera.pos.z) < 25) {
            matrix_upload();
            glLineWidth(1.0F);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnableClientState(GL_VERTEX_ARRAY);

            LineRasterizer i = is_local && local_player.drag_active ?
            cube_line(
                local_player.drag.x,
                local_player.drag.z,
                63 - local_player.drag.y,
                pos[X], pos[Z], 63 - pos[Y]
            ) :
            cube_rasterizer(
                pos[X], pos[Z], 63 - pos[Y]
            );

            for (; !i.exhausted; rasterizer_next(&i)) {
                int x = i.x, y = 63 - i.z, z = i.y;

                // see “handlePacketBlockLine” in “src/network.c”
                int avail = is_local ? min(local_player.blocks, BLOCKLINE_MAX_LENGTH) : 50;

                if (i.index < avail && isdestructible(x, y, z))
                    glColor3f(1.0F, 1.0F, 1.0F);
                else
                    glColor3f(1.0F, 0.0F, 0.0F);

                drawCubeEdges(x, y, z);
            }

            glDisableClientState(GL_VERTEX_ARRAY);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }

        bool item_disabled = window_time() - players[local_player.id].item_disabled < 0.3F;

        if (item_disabled) {
            players[local_player.id].item_showup = window_time();
            if (HASBIT(players[local_player.id].input.buttons, BUTTON_PRIMARY))
                players[local_player.id].start.lmb = window_time() + 0.5F;
            if (HASBIT(players[local_player.id].input.buttons, BUTTON_SECONDARY))
                players[local_player.id].start.rmb = window_time() + 0.5F;
        }

        if (!item_disabled || settings.render_player) {
            if (hud_active->render_localplayer) {
                float tmp2 = players[local_player.id].physics.eye.y;
                players[local_player.id].physics.eye.y = last_cy;
                if (camera.mode == CAMERAMODE_FPS && !settings.render_player)
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
        if (camera.noclip && camera.mode == CAMERAMODE_SPECTATOR) {}
        else if (!map_isair(camera.pos.x, camera.pos.y, camera.pos.z)) {
            const float brightness = 0.15F;

            RGBA4i color = map_get(camera.pos.x, camera.pos.y, camera.pos.z);
            float r = color.r / 255.0F, g = color.g / 255.0F, b = color.b / 255.0F;

            glClearColor(brightness * r, brightness * g, brightness * b, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
        }
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

        if (ctx != NULL) {
            hud_active->ctx->style->padding        = 10 * scale - 5;
            hud_active->ctx->style->spacing        = 8  * scale - 4;
            hud_active->ctx->style->title_height   = 48 * scale - 24;
            hud_active->ctx->style->scrollbar_size = 12 * scale;
            hud_active->ctx->style->thumb_size     = 8  * scale;

            mu_begin(ctx);
        }

        hud_active->render_2D(ctx, scale);

        if (ctx != NULL) {
            mu_end(ctx);

            glEnable(GL_BLEND);
            glEnable(GL_SCISSOR_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            mu_Command * cmd = NULL;
            while (mu_next_command(ctx, &cmd)) {
                switch (cmd->type) {
                    case MU_COMMAND_TEXT:
                        glColor4ub(cmd->text.color.r, cmd->text.color.g, cmd->text.color.b, cmd->text.color.a);

                        if (cmd->text.font != NULL) font_select(cmd->text.font);
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

                            if (img != NULL) {
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

void init(void) {
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

    size_t destlen = strlen(game_chat_input);

    while (text != end) {
        char buff[5] = {0};

        size_t size = decodeSize(UTF8, text[0]);
        if (!isprintuni(text[0])) goto skip; // non-printable ASCII or invalid UTF-8

        // everything else assumed to be printable
        switch (size) {
            case 4: buff[3] = text[3];
            case 3: buff[2] = text[2];
            case 2: buff[1] = text[1];
            case 1: buff[0] = text[0];
        }

        if (hud_active->ctx) mu_input_text(hud_active->ctx, buff);

        if (chat_input_mode != CHAT_NO_INPUT)
        if (destlen + size < sizeof(game_chat_input)) {
            strcpy(&game_chat_input[destlen], buff);
            destlen += size;
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
        settings.windowed = !settings.windowed;
        window_videomode(settings.windowed);
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

        hud_show_popup("Saved screenshot as %s", pic_name);
    }

    if (key == WINDOW_KEY_SAVE_MAP && action == WINDOW_PRESS) { // save map
        time_t save_time;
        time(&save_time);
        char save_name[128];
        sprintf(save_name, "vxl/%ld.vxl", (long) save_time);

        map_save_file(save_name);

        hud_show_popup("Saved map as %s", save_name);
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

void deinit(void) {
    deque_free(&game_chat);
    deque_free(&game_killfeed);

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

void game_idle(double dt) {
    static double physics_time_fixed = 0.0F;
    static double physics_time_fast  = 0.0F;

    physics_time_fast  += dt;
    physics_time_fixed += dt;

    // these run at exactly ~60fps
    #define PHYSICS_STEP_TIME (1.0 / 60.0)
    while (physics_time_fixed >= PHYSICS_STEP_TIME) {
        physics_time_fixed -= PHYSICS_STEP_TIME;
        player_update_position(PHYSICS_STEP_TIME); // just physics tick
        grenade_update(PHYSICS_STEP_TIME);
    }

    // these run at min. ~60fps but as fast as possible
    double step = fmin(dt, PHYSICS_STEP_TIME);
    while (step > 0 && physics_time_fast >= step) {
        physics_time_fast -= step;
        player_update_orientation(step); // smooth orientation update
        camera_update(step);
        tracer_update(step);
        particle_update(step);
        map_collapsing_update(step);
    }

    sound_update();
    network_update();
    rpc_update();
}

static inline bool startswith(const char * prefix, const char * str)
{ return strncmp(prefix, str, strlen(prefix)) == 0; }

#define MATCH(x, y) if (!strcmp((x), (y)))
#define THROW(retcode, ...) { printf(__VA_ARGS__); return retcode; }

int game_main(int argc, char ** argv) {
    const char * vxl_file = NULL, * default_server = NULL;

    for (int i = 1; i < argc; i++) {
        if (startswith("-aos://", argv[i])) {
            default_server = argv[i] + 1;
        } else MATCH(argv[i], "--vxl") {
            if (argc <= ++i) THROW(-1, "The “--vxl” option requires an argument.\n")
            else vxl_file = argv[i];
        } else MATCH(argv[i], "--help") {
            THROW(0, "Usage: %s -aos://<ip>:<port>"
                     " --vxl <file.vxl> --config <file.ini> --team <team> --weapon <weapon>"
                     " --serverlist <url> --newslist <url> --offline --help --version\n", argv[0]);
        } else MATCH(argv[i], "--version") {
            THROW(0, "TigerSpades %s (%s)\n"
                     "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>\n"
                     "This is free software: you are free to change and redistribute it.\n"
                     "There is NO WARRANTY, to the extent permitted by law.\n", BSVERSION, GIT_COMMIT_HASH);
        } else MATCH(argv[i], "--serverlist") {
            if (argc <= ++i) THROW(-1, "The “--serverlist” option requires an argument.\n")
            else strnzcpy(serverlist_url, argv[i], sizeof(serverlist_url));
        } else MATCH(argv[i], "--newslist") {
            if (argc <= ++i) THROW(-1, "The “--newslist” option requires an argument.\n")
            else strnzcpy(newslist_url, argv[i], sizeof(newslist_url));
        } else MATCH(argv[i], "--team") {
            if (argc <= ++i) THROW(-1, "The “--team” option requires an argument.\n")
            else MATCH(argv[i], "1") default_team = TEAM1;
            else MATCH(argv[i], "2") default_team = TEAM2;
            else MATCH(argv[i], "3") default_team = TEAM_SPECTATOR;
            else THROW(-2, "Unknown team (expected 1, 2, or 3).\n");
        } else MATCH(argv[i], "--weapon") {
            if (argc <= ++i) THROW(-1, "The “--weapon” option requires an argument.\n")
            else MATCH(argv[i], "rifle")   default_gun = WEAPON_RIFLE;
            else MATCH(argv[i], "smg")     default_gun = WEAPON_SMG;
            else MATCH(argv[i], "shotgun") default_gun = WEAPON_SHOTGUN;
            else THROW(-2, "Unknown weapon name (expected rifle, smg, or shotgun).\n");
        } else MATCH(argv[i], "--config") {
            if (argc <= ++i) THROW(-1, "The “--config” option requires an argument.\n")
            else config_filepath = argv[i];
        } else MATCH(argv[i], "--offline") {
            offline = true;
        } else {
            THROW(-3, "Unknown option “%s”.\n", argv[i]);
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

    log_info("TigerSpades " BSVERSION);

    config_init();

    window_init("TigerSpades " BSVERSION, &argc, argv);

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

    if (vxl_file != NULL) {
        if (file_exists(vxl_file))
            load_map(vxl_file);
        else {
            log_error("Error: file not found: %s", vxl_file);
            return -4;
        }
    } else if (default_server != NULL) {
        if (!network_connect_string(default_server, VER07X)) {
            log_error("Error: connection failed (use --help for instructions)");
            return -4;
        } else {
            log_info("Connection to %s successful", default_server);
            hud_change(&hud_mapload);
        }
    }

    return 1;
}
