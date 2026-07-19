/*
    Copyright © 2017–2023 ByteBit
    Copyright © 2018 vuolen
    Copyright © 2018 NotAFile
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

#include <string.h>
#include <limits.h>

#include <lodepng/lodepng.h>

#include <bs/file.h>
#include <bs/hud.h>
#include <bs/rpc.h>
#include <bs/map.h>
#include <bs/camera.h>
#include <bs/ping.h>
#include <bs/chunk.h>
#include <bs/particle.h>

static char serverlist_input[128];

void hud_serverlist_refresh(void) {
    rpc_seti(RPC_VALUE_SLOTS, 0);

    *serverlist_input = 0;
    ping_refresh();
}

static void hud_serverlist_init(void) {
    window_textinput(1);
    window_mousemode(WINDOW_CURSOR_ENABLED);

    window_title(NULL);
}

void load_map(const char * filepath) {
    void * data = file_load(filepath);
    map_vxl_load(data, file_size(filepath));
    free(data);

    chunk_rebuild_all();

    players[local_player.id].pos.x = map_size_x / 2.0F;
    players[local_player.id].pos.y = map_size_y - 1.0F;
    players[local_player.id].pos.z = map_size_z / 2.0F;

    local_hit_effects = true;

    camera.mode = CAMERAMODE_FPS;

    window_title(filepath);
    hud_change(&hud_ingame);
}

const char * hud_serverlist_popup = NULL;

static void server_c(char * address, char * name, GameVersion version) {
    if (file_exists(address)) load_map(address);
    else {
        window_title(name);
        if (name && address) {
            rpc_setv(RPC_VALUE_SERVERNAME, name);
            rpc_setv(RPC_VALUE_SERVERURL, address);
            rpc_seti(RPC_VALUE_SLOTS, 32);
        } else {
            rpc_seti(RPC_VALUE_SLOTS, 0);
        }

        if (network_connect_string(address, version))
            hud_change(&hud_mapload);
        else
            hud_serverlist_popup = "Unable to initiate connection";
    }
}

static Texture * hud_serverlist_ui_images(int icon_id, bool * resize) {
    UNUSED(resize);

    if (icon_id >= 32) {
        News * current = newslist;

        int index = 32;
        while (current) {
            if (index == icon_id)
                return current->image;

            index++;
            current = current->next;
        }
    }

    switch (icon_id) {
        case MU_ICON_EXPANDED:  return texture(TEXTURE_UI_EXPANDED);
        case MU_ICON_COLLAPSED: return texture(TEXTURE_UI_COLLAPSED);
        default:                return NULL;
    }
}

static void hud_sort_button_render(mu_Context * ctx, float scale, const char * name, ServerlistComparator cmp) {
    UNUSED(scale);

    if (serverlist_comparator == cmp)
        mu_text_color(ctx, 255, 255, 0);

    if (mu_button_ex(ctx, name, 0, MU_OPT_ALIGNCENTER)) {
        serverlist_lock();

        if (cmp != serverlist_comparator) {
            serverlist_comparator = cmp;
            serverlist_descending = true;
        } else serverlist_descending = !serverlist_descending;

        serverlist_sort();

        serverlist_unlock();
    }

    mu_text_color_default(ctx);
}

static void hud_serverlist_render(mu_Context * ctx, float scale) {
    char total_str[128]; sprintf(total_str, server_count > 0 ? "%zu players on %zu servers" : "No servers", player_count, server_count);

    char * join_address = NULL, * join_name = NULL; GameVersion join_version = VER07X;

    if (hud_header_render(ctx, scale, total_str)) {
        mu_layout_row(ctx, 1, (int[]) {-1}, settings.window_height * 0.3F);

        if (newslist != NULL && settings.show_news) {
            mu_begin_panel(ctx, "News");
            mu_layout_row(ctx, 0, NULL, 0);

            int index = 0;

            for (News * current = newslist; current != NULL; current = current->next) {
                if (current->imgdata != NULL) {
                    // This data arrives from the other thread, so we have to do OpenGL things here.

                    uint32_t * buffer; unsigned int width, height;
                    lodepng_decode32((unsigned char **) &buffer, &width, &height, (uint8_t *) current->imgdata, current->imgsize);

                    current->image = texture_alloc();
                    texture_create_buffer(current->image, "image", width, height, buffer, true);
                    texture_filter(current->image, TEXTURE_FILTER_LINEAR);

                    free(buffer);

                    free(current->imgdata);
                    current->imgdata = NULL;
                    current->imgsize = 0;
                }

                mu_layout_begin_column(ctx);
                float size = settings.window_height * 0.3F - ctx->text_height(ctx->style->font) * 4.125F;
                mu_layout_row(ctx, 1, (int[]) {size * current->tile_size}, size);

                if (mu_button_ex(ctx, NULL, 32 + index, MU_OPT_NOFRAME)) {
                    if (!strncmp("aos://", current->url, 6)) {
                        join_address = current->url;
                        join_name    = current->caption;
                    } else file_url(current->url);
                }

                mu_layout_height(ctx, 0);
                mu_text_color(ctx, current->color.r, current->color.g, current->color.b);
                mu_text(ctx, current->caption);
                mu_text_color_default(ctx);
                mu_layout_end_column(ctx);

                index++;
            }

            mu_end_panel(ctx);
        }

        int a = ctx->text_width(ctx->style->font, "Refresh", 0) * 1.6F;
        int b = ctx->text_width(ctx->style->font, "Join", 0) * 2.0F;
        mu_layout_row(ctx, 3, (int[]) {-a - b, -a, -1}, 0);

        if (mu_textbox(ctx, serverlist_input, sizeof(serverlist_input)) & MU_RES_SUBMIT)
            join_address = serverlist_input;

        if (mu_button_ex(ctx, "Join", 0, MU_OPT_ALIGNCENTER))
            join_address = serverlist_input;

        if (mu_button_ex(ctx, "Refresh", 0, MU_OPT_ALIGNCENTER))
            hud_serverlist_refresh();

        mu_layout_row(ctx, 1, (int[]) {-1}, -1);

        mu_begin_panel(ctx, "Servers");
        int width = mu_get_current_container(ctx)->body.w - mu_text_width(ctx->style->font, "Ping", 0);

        float A = fmaxf(mu_text_width(ctx->style->font, "Players",  0), 0.120F * width);
        float B = fmaxf(mu_text_width(ctx->style->font, "Name",     0), 0.415F * width);
        float C = fmaxf(mu_text_width(ctx->style->font, "Map",      0), 0.220F * width);
        float D = fmaxf(mu_text_width(ctx->style->font, "Mode",     0), 0.120F * width);

        int flag_width = ctx->style->size.y + ctx->style->padding * 2;
        mu_layout_row(ctx, 5, (int[]) {A, B, C, D, -1}, 0);

        hud_sort_button_render(ctx, scale, "Players", serverlist_sort_players);
        hud_sort_button_render(ctx, scale, "Name",    serverlist_sort_name);
        hud_sort_button_render(ctx, scale, "Map",     serverlist_sort_map);
        hud_sort_button_render(ctx, scale, "Mode",    serverlist_sort_mode);
        hud_sort_button_render(ctx, scale, "Ping",    serverlist_sort_ping);

        mu_layout_row(ctx, 6, (int[]) {A, flag_width, B - flag_width - ctx->style->spacing * 2, C, D, -1}, 0);

        serverlist_lock();

        if (server_count > 0) {
            for (size_t k = 0; k < server_count; k++) {
                if (strstr(serverlist[k]->name, serverlist_input) || strstr(serverlist[k]->identifier, serverlist_input)
                 || strstr(serverlist[k]->map,  serverlist_input) || strstr(serverlist[k]->gamemode,   serverlist_input)) {
                    bool shadowed = serverlist[k]->current <= 0 || serverlist[k]->max <= serverlist[k]->current;
                    int f = shadowed ? 2 : 1;

                    if (serverlist[k]->current >= 0)
                        sprintf(total_str, "%i/%i", serverlist[k]->current, serverlist[k]->max);
                    else
                        strcpy(total_str, "-");

                    mu_push_id(ctx, &serverlist[k], sizeof(ServerEntry *));

                    bool join = false;

                    float ratio = ((float) serverlist[k]->current) / ((float) serverlist[k]->max);

                    if (ratio >= 1.0)
                        mu_text_color(ctx, 255 / f, 0, 0);
                    else if (ratio >= 0.75)
                        mu_text_color(ctx, 255 / f, 255 / f, 0);
                    else
                        mu_text_color(ctx, 230 / f, 230 / f, 230 / f);

                    if (mu_button_ex(ctx, total_str, 0, MU_OPT_NOFRAME | MU_OPT_ALIGNCENTER))
                        join = true;

                    mu_text_color(ctx, 230 / f, 230 / f, 230 / f);

                    if (mu_button_ex(ctx, "", texture_flag_index(serverlist[k]->country) + HUD_FLAG_INDEX_START,
                                    MU_OPT_NOFRAME))
                        join = true;

                    if (mu_button_ex(ctx, serverlist[k]->name, 0, MU_OPT_NOFRAME))
                        join = true;

                    if (mu_button_ex(ctx, serverlist[k]->map, 0, MU_OPT_NOFRAME))
                        join = true;

                    if (mu_button_ex(ctx, serverlist[k]->gamemode, 0, MU_OPT_NOFRAME | MU_OPT_ALIGNCENTER))
                        join = true;

                    if (serverlist[k]->ping >= 0) {
                        if (serverlist[k]->ping < 110)
                            mu_text_color(ctx, 0, 255 / f, 0);
                        else if (serverlist[k]->ping < 200)
                            mu_text_color(ctx, 255 / f, 255 / f, 0);
                        else
                            mu_text_color(ctx, 255 / f, 0, 0);
                    }

                    sprintf(total_str, "%i", serverlist[k]->ping);
                    if (mu_button_ex(ctx, serverlist[k]->ping >= 0 ? total_str : "?", 0,
                                     MU_OPT_NOFRAME | MU_OPT_ALIGNCENTER))
                        join = true;

                    mu_pop_id(ctx);

                    if (join) {
                        join_address = serverlist[k]->identifier;
                        join_name    = serverlist[k]->name;
                        join_version = serverlist[k]->version;
                    }
                }
            }
        } else {
            mu_layout_row(ctx, 1, (int[]) {-1}, 0);

            const char * status = ping_status();

            if (status != NULL)
                mu_button_ex(ctx, status, 0, MU_OPT_NOFRAME | MU_OPT_ALIGNCENTER);
        }

        serverlist_unlock();

        mu_text_color_default(ctx);
        mu_end_panel(ctx);

        mu_end_window(ctx);
    }

    if (hud_serverlist_popup != NULL) {
        int w = 380 * scale, h = 110 * scale;
        int x = (settings.window_width - w) / 2;
        int y = (settings.window_height - h) / 2;

        if (mu_begin_window_ex(ctx, "Disconnected", mu_rect(x, y, w, h), MU_OPT_NORESIZE | MU_OPT_NOCLOSE)) {
            mu_Container * cnt = mu_get_current_container(ctx);
            cnt->zindex = INT_MAX;

            mu_layout_row(ctx, 1, (int[]) {-1}, 0);
            mu_text(ctx, "You have been disconnected from the server.");

            mu_layout_row(ctx, 2, (int[]) {ctx->text_width(ctx->style->font, "Reason: ", 0), -1}, 0);

            mu_text_color(ctx, 200, 200, 200);
            mu_text(ctx, "Reason:");

            mu_text_color_default(ctx);
            mu_text(ctx, hud_serverlist_popup);

            int width = cnt->body.w;
            mu_layout_row(ctx, 2, (int[]) {0.75F * width, -1}, 0);
            mu_layout_next(ctx);

            if (mu_button(ctx, "Close")) hud_serverlist_popup = NULL;

            mu_end_window(ctx);
        }
    }

    if (join_address) server_c(join_address, join_name, join_version);
}

static void hud_serverlist_touch(void * finger, int action, float x, float y, float dx, float dy) {
    UNUSED(finger); UNUSED(action); UNUSED(dx); UNUSED(dy);

    window_setmouseloc(x, y);
    /*switch (action) {
        case TOUCH_DOWN: hud_serverlist_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_PRESS, 0); break;
        case TOUCH_MOVE: hud_serverlist_mouselocation(x, y); break;
        case TOUCH_UP: hud_serverlist_mouseclick(x, y, WINDOW_MOUSE_LMB, WINDOW_RELEASE, 0); break;
    }*/
}

static void hud_serverlist_keyboard(int key, int action, int mods, int internal) {
    UNUSED(mods); UNUSED(internal);

    if (action == WINDOW_PRESS && key == WINDOW_KEY_ESCAPE)
        if (!map_empty()) hud_change(&hud_ingame);
}

HUD hud_serverlist = {
    hud_serverlist_init,
    NULL,
    hud_serverlist_render,
    hud_serverlist_keyboard,
    NULL,
    NULL,
    NULL,
    hud_serverlist_touch,
    NULL,
    NULL,
    hud_serverlist_ui_images,
    0,
    0,
    NULL,
};
