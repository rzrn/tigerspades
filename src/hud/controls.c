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

#include <string.h>

#include <bs/hud.h>
#include <bs/map.h>

static int * hud_controls_edit;

static void hud_controls_init(void) {
    window_textinput(1);
    window_mousemode(WINDOW_CURSOR_ENABLED);

    hud_controls_edit = NULL;
}

static void hud_controls_render(mu_Context * ctx, float scale) {
    if (hud_header_render(ctx, scale, BSSUMMARY)) {
        mu_layout_row(ctx, 1, (int[]) {-1}, -1);
        mu_begin_panel(ctx, "Controls");

        bool expanded = false;

        for (WindowKey key = WINDOW_KEY_FIRST; key <= WINDOW_KEY_LAST; key++) {
            ConfigKey * e = config_key(key);

            if (e->display != NULL) {
                if (e->category != NULL) expanded = mu_header_ex(ctx, e->category, MU_OPT_EXPANDED);

                if (expanded) {
                    int width = mu_get_current_container(ctx)->body.w;
                    if (e->keycode != e->original) {
                        mu_layout_row(ctx, 4,
                                      (int[]) {0.50F * width, ctx->text_width(ctx->style->font, "Reset", 0) * 1.5F,
                                               -0.05F * width, -1},
                                      0);
                    } else {
                        mu_layout_row(ctx, 3, (int[]) {0.50F * width, -0.05F * width, -1}, 0);
                    }

                    mu_push_id(ctx, &key, sizeof(WindowKey));
                    mu_text(ctx, e->display);

                    if (e->keycode != e->original && mu_button(ctx, "Reset")) {
                        e->keycode = e->original;
                        config_save();
                    }

                    char name[32]; window_keyname(e->keycode, name, sizeof(name));

                    if (hud_controls_edit == &e->keycode)
                        mu_text_color(ctx, 255, 0, 0);

                    if (mu_button(ctx, name))
                        hud_controls_edit = hud_controls_edit == &e->keycode ? NULL : &e->keycode;

                    mu_text_color_default(ctx);

                    if (mu_begin_popup(ctx, "Help")) {
                        mu_layout_row(ctx, 1, (int[]) {ctx->text_width(ctx->style->font, e->name, 0)}, 0);
                        mu_text(ctx, e->name);
                        mu_end_popup(ctx);
                    }

                    if (mu_button(ctx, "?"))
                        mu_open_popup(ctx, "Help");

                    mu_pop_id(ctx);
                }
            }
        }

        if (mu_header_ex(ctx, "Key bindings", MU_OPT_EXPANDED)) {
            int width = mu_get_current_container(ctx)->body.w;
            mu_layout_row(ctx, 3, (int[]) {0.50F * width, -0.05F * width, -1}, 0);

            int idel = -1;

            for (int k = 0; k < list_size(&config_keybind); k++) {
                Keybind * keybind = list_get(&config_keybind, k);

                mu_textbox(ctx, keybind->value, sizeof(keybind->value));

                char keyname[32]; window_keyname(keybind->key, keyname, sizeof(keyname));

                mu_push_id(ctx, &k, sizeof(k));

                if (hud_controls_edit == &keybind->key)
                    mu_text_color(ctx, 255, 0, 0);

                if (mu_button(ctx, keyname))
                    hud_controls_edit = (hud_controls_edit == &keybind->key) ? NULL : &keybind->key;

                mu_text_color_default(ctx);

                if (mu_button(ctx, "X")) idel = k;

                mu_pop_id(ctx);
            }

            char buf[] = "\0"; mu_textbox_ex(ctx, buf, 0, MU_OPT_NOINTERACT);
            mu_button_ex(ctx, NULL, 0, MU_OPT_NOINTERACT);

            if (mu_button(ctx, "+")) {
                Keybind * keybind = list_add(&config_keybind, NULL);
                keybind->key = 0; memset(keybind->value, 0, sizeof(keybind->value));
            }

            mu_layout_row(ctx, 3, (int[]) {0.25F * width, 0.50F * width, -1}, 0);

            mu_layout_next(ctx);

            if (mu_button(ctx, "Save"))
                config_save();

            if (idel >= 0) list_remove(&config_keybind, idel);
        }

        mu_end_panel(ctx);

        mu_end_window(ctx);
    }
}

static void hud_controls_touch(void * finger, int action, float x, float y, float dx, float dy) {
    UNUSED(finger); UNUSED(action); UNUSED(dx); UNUSED(dy);

    window_setmouseloc(x, y);
}

static void hud_controls_keyboard(int key, int action, int mods, int internal) {
    UNUSED(mods);

    if (hud_controls_edit) {
        *hud_controls_edit = internal;
        hud_controls_edit = NULL;
        config_save();
    } else if (action == WINDOW_PRESS && key == WINDOW_KEY_ESCAPE)
        if (!map_empty()) hud_change(&hud_ingame);
}

HUD hud_controls = {
    hud_controls_init,
    NULL,
    hud_controls_render,
    hud_controls_keyboard,
    NULL,
    NULL,
    NULL,
    hud_controls_touch,
    NULL,
    NULL,
    hud_ui_images,
    0,
    0,
    NULL,
};
