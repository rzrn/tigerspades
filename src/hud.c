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

#include <bs/hud.h>
#include <bs/weapon.h>
#include <bs/opengl.h>

HUD * hud_active;

Texture * hud_ui_images(int icon_id, bool * resize) {
    UNUSED(resize);

    switch (icon_id) {
        case MU_ICON_EXPANDED:  return texture(TEXTURE_UI_EXPANDED);
        case MU_ICON_COLLAPSED: return texture(TEXTURE_UI_COLLAPSED);
        default:                return NULL;
    }
}

static int hud_render_tab_button(mu_Context * ctx, float scale, const char * tabname, HUD * tabptr) {
    UNUSED(scale);

    if (hud_active == tabptr) mu_text_color(ctx, 255, 255, 0);

    int retval = mu_button_ex(ctx, tabname, 0, MU_OPT_ALIGNCENTER | (hud_active == tabptr ? MU_OPT_NOINTERACT : 0));
    mu_text_color_default(ctx);

    if (retval) hud_change(tabptr);

    return retval;
}

int hud_header_render(mu_Context * ctx, float scale, const char * text) {
    glColor3f(0.5F, 0.5F, 0.5F);
    float t = window_time() * 0.03125F;
    texture_draw_sector(texture(TEXTURE_UI_BG), 0.0F, settings.window_height, settings.window_width, settings.window_height, t,
                        t, settings.window_width / 512.0F, settings.window_height / 512.0F);

    mu_Rect frame = mu_rect(settings.window_width * 0.125F, 0, settings.window_width * 0.75F, settings.window_height);

    int retval = mu_begin_window_ex(ctx, "Main", frame, MU_OPT_NOFRAME | MU_OPT_NOTITLE | MU_OPT_NORESIZE);

    if (retval) {
        mu_Container * cnt = mu_get_current_container(ctx); cnt->rect = frame;

        int width = cnt->body.w;

        float A = fmaxf(mu_text_width(ctx->style->font, "Servers",  0), 0.166F * width);
        float B = fmaxf(mu_text_width(ctx->style->font, "Settings", 0), 0.166F * width);
        float C = fmaxf(mu_text_width(ctx->style->font, "Controls", 0), 0.166F * width);

        mu_layout_row(ctx, 4, (int[]) {A, B, C, -1}, 0);

        if (hud_render_tab_button(ctx, scale, network_connected ? "Disconnect" : "Servers", &hud_serverlist)) {
            if (network_connected) {
                hud_serverlist_refresh();
                network_disconnect();
            }
        }

        hud_render_tab_button(ctx, scale, "Settings", &hud_settings);
        hud_render_tab_button(ctx, scale, "Controls", &hud_controls);

        mu_text_color_default(ctx);

        if (network_connected) {
            char play_time[128]; sprintf(play_time, "Playing for %i min. %02i sec.", (int) window_time() / 60, (int) window_time() % 60);
            mu_button_ex(ctx, play_time, 0, MU_OPT_ALIGNRIGHT | MU_OPT_NOINTERACT);
        } else mu_button_ex(ctx, text, 0, MU_OPT_ALIGNRIGHT | MU_OPT_NOINTERACT);
    }

    return retval;
}

void hud_init(void) {
    mu_Context * ctx = malloc(sizeof(mu_Context)); mu_init(ctx);
    hud_serverlist.ctx = hud_settings.ctx = hud_controls.ctx = ctx;

    ctx->text_width                          = mu_text_width;
    ctx->text_height                         = mu_text_height;
    ctx->style->font                         = font_primary;
    ctx->style->colors[MU_COLOR_BUTTONHOVER] = mu_color(95, 95, 70, 255);
    ctx->style->colors[MU_COLOR_PANELBG]     = mu_color(10, 10, 10, 192);
    ctx->style->colors[MU_COLOR_SCROLLTHUMB] = mu_color(128, 128, 128, 255);

    hud_change(&hud_serverlist);
}

void hud_change(HUD * new) {
    if (hud_active == new)
        return;

    button_map.lmb = button_map.rmb = button_map.mmb = false;
    window_key_reset_togglestates();

    hud_active = new;

    if (hud_active->ctx) {
        mu_set_focus(hud_active->ctx, 0);

        hud_active->ctx->mouse_down    = 0;
        hud_active->ctx->mouse_pressed = 0;
        hud_active->ctx->key_down      = 0;
        hud_active->ctx->key_pressed   = 0;
    }

    if (hud_active->init)
        hud_active->init();
}
