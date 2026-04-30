/*
    Copyright © 2017–2023 ByteBit
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

#include <bs/network.h>
#include <bs/opengl.h>
#include <bs/hud.h>

static void hud_mapload_init(void) {
    window_mousemode(WINDOW_CURSOR_ENABLED);
}

static inline const char * ellipsis(void) {
    static const char * suffix[] = {"   ", ".  ", ".. ", "..."};
    size_t index = ((size_t) (window_time() / 0.2F)) % lengthof(suffix);

    return suffix[index];
}

static void hud_mapload_render(mu_Context * ctx, float scale) {
    UNUSED(ctx);

    glColor3f(1.0F, 1.0F, 1.0F);
    texture_draw(
        texture(TEXTURE_SPLASH), (settings.window_width - settings.window_height * 4.0F / 3.0F * 0.7F) * 0.5F,
        settings.window_height - 40 * scale, settings.window_height * 4.0F / 3.0F * 0.7F, settings.window_height * 0.7F
    );

    if (network_map_transfer) {
        float progress = compressed_chunk_data_estimate > 0 ? ((float) compressed_chunk_data_offset / (float) compressed_chunk_data_estimate) : 0.0F;
        progress = clamp(0.0F, 1.0F, progress);

        glColor3ub(68, 68, 68);
        texture_draw(texture(TEXTURE_WHITE), (settings.window_width - 440.0F * scale) / 2.0F + 440.0F * scale * progress,
                     settings.window_height * 0.25F, 440.0F * scale * (1.0F - progress), 20.0F * scale);
        glColor3ub(255, 255, 50);
        texture_draw(texture(TEXTURE_WHITE), (settings.window_width - 440.0F * scale) / 2.0F, settings.window_height * 0.25F,
                     440.0F * scale * progress, 20.0F * scale);
    }

    char buff[128];
    if (network_map_transfer)
        sprintf(buff, "Receiving %zu KiB / %zu KiB", compressed_chunk_data_offset / 1024, compressed_chunk_data_estimate / 1024);
    else if (network_connected)
        sprintf(buff, "Awaiting for state%s", ellipsis());
    else
        sprintf(buff, "Connecting%s", ellipsis());

    glColor3ub(69, 69, 69);
    font_centered(settings.window_width / 2.0F, settings.window_height * 0.25F - 20.0F * scale, 2.0F * scale, buff, ASCII);

    font_select(font_secondary);
    glColor3f(1.0F, 1.0F, 0.0F);
    font_render(0.0F, 16.0F * scale, 1.0F * scale, "Created by ByteBit, visit https://github.com/xtreme8000/BetterSpades", ASCII);
    font_select(font_primary);
}

static void hud_mapload_keyboard(int key, int action, int mods, int internal) {
    UNUSED(mods); UNUSED(internal);

    if (action == WINDOW_PRESS && key == WINDOW_KEY_ESCAPE) {
        network_disconnect();
        hud_change(&hud_serverlist);
    }
}

HUD hud_mapload = {
    .init                = hud_mapload_init,
    .render_3D           = NULL,
    .render_2D           = hud_mapload_render,
    .input_keyboard      = hud_mapload_keyboard,
    .input_mouselocation = NULL,
    .input_mouseclick    = NULL,
    .input_mousescroll   = NULL,
    .input_touch         = NULL,
    .focus               = NULL,
    .hover               = NULL,
    .ui_images           = NULL,
    .render_world        = false,
    .render_localplayer  = false,
    .ctx                 = NULL,
};
