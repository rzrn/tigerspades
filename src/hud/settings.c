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
#include <limits.h>

#include <bs/config.h>
#include <bs/sound.h>
#include <bs/hud.h>
#include <bs/map.h>

static void hud_settings_init(void) {
    window_textinput(1);
    window_mousemode(WINDOW_CURSOR_ENABLED);

    memcpy(&settings_tmp, &settings, sizeof(Options));
}

static inline int defaults_round(Setting * setting, float value) {
    int k; for (k = setting->numdefs - 1; 0 < k && value < setting->valdefs[k]; k--);

    return k;
}

static int int_slider_defaults(mu_Context * ctx, Setting * setting) {
    int * value = setting->value;

    mu_push_id(ctx, &setting, sizeof(setting));

    float slider = defaults_round(setting, *value);
    int res = mu_slider_ex(ctx, &slider, 0, setting->numdefs - 1, 0, "", MU_OPT_ALIGNCENTER);

    if (res & MU_RES_CHANGE) *value = setting->valdefs[(int) round(slider)];

    if (setting->label != NULL) {
        char buf[64]; setting->label(buf, sizeof(buf), value);
        mu_draw_control_text(ctx, buf, ctx->last_rect, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
    }

    mu_pop_id(ctx);
    return res;
}

static int int_slider(mu_Context * ctx, Setting * setting) {
    int * value = setting->value;

    mu_push_id(ctx, &setting, sizeof(setting));

    float slider = *value;
    int res = mu_slider_ex(ctx, &slider, setting->mini, setting->maxi, 0, "", MU_OPT_ALIGNCENTER);

    if (res & MU_RES_CHANGE) *value = round(slider);

    char buf[64];

    if (setting->label != NULL)
        setting->label(buf, sizeof(buf), value);
    else
        snprintf(buf, sizeof(buf), "%d", *value);

    mu_draw_control_text(ctx, buf, ctx->last_rect, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);

    mu_pop_id(ctx);
    return res;
}

static int int_number(mu_Context * ctx, int * value) {
    float tmp = *value;
    mu_push_id(ctx, &value, sizeof(value));
    int res = mu_number_ex(ctx, &tmp, 1, "%.0f", MU_OPT_ALIGNCENTER);
    mu_pop_id(ctx);
    *value = max(round(tmp), 0);
    return res;
}

static int float_slider(mu_Context * ctx, Setting * setting) {
    float * value = setting->value;

    mu_push_id(ctx, &setting, sizeof(setting));

    int res = mu_slider_ex(ctx, value, setting->minf, setting->maxf, 0, "", MU_OPT_ALIGNCENTER);

    char buf[64];

    if (setting->label != NULL)
        setting->label(buf, sizeof(buf), value);
    else
        snprintf(buf, sizeof(buf), "%.2f", *value);

    mu_draw_control_text(ctx, buf, ctx->last_rect, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);

    mu_pop_id(ctx);
    return res;
}

static void hud_bool(mu_Context * ctx, Setting * setting) {
    mu_push_id(ctx, &setting, sizeof(setting));

    bool * value = setting->value; char buf[64];

    if (setting->label != NULL)
        setting->label(buf, sizeof(buf), value);
    else
        snprintf(buf, sizeof(buf), *value ? "Yes" : "No");

    if (mu_button(ctx, buf))
        *value = !(*value);

    mu_pop_id(ctx);
}

static void hud_enum(mu_Context * ctx, Setting * setting) {
    mu_push_id(ctx, &setting, sizeof(setting));

    int * value = setting->value;

    char buf[64]; setting->label(buf, sizeof(buf), value);

    if (mu_button(ctx, buf)) {
        for (size_t i = 0; i < setting->numdefs; i++) {
            if (setting->valdefs[i] == *value) {
                *value = setting->valdefs[(i + 1) % setting->numdefs];
                break;
            }
        }
    }

    mu_pop_id(ctx);
}

static void hud_settings_render(mu_Context * ctx, float scale) {
    if (hud_header_render(ctx, scale, BSSUMMARY)) {
        mu_layout_row(ctx, 1, (int[]) {-1}, -1);
        mu_begin_panel(ctx, "Settings");

        int width = mu_get_current_container(ctx)->body.w;

        bool expanded = false;

        for (Setting * a = config_settings_begin; a != config_settings_end; a++) {
            if (a->category != NULL) expanded = mu_header_ex(ctx, a->category, MU_OPT_EXPANDED);

            if (expanded && a->display != NULL) {
                mu_layout_row(ctx, 3, (int[]) {0.50F * width, -0.05F * width, -1}, 0);
                mu_text(ctx, a->display);

                switch (a->type) {
                    case CONFIG_TYPE_BOOLEAN: {
                        hud_bool(ctx, a);
                        break;
                    }

                    case CONFIG_TYPE_STRING: {
                        mu_textbox(ctx, a->value, a->size);
                        break;
                    }

                    case CONFIG_TYPE_ENUM: {
                        hud_enum(ctx, a);
                        break;
                    }

                    case CONFIG_TYPE_INT: {
                        if (a->numdefs > 0) {
                            int_slider_defaults(ctx, a);
                        } else if (a->maxi == INT_MAX) {
                            int_number(ctx, a->value);
                        } else {
                            int_slider(ctx, a);
                        }

                        break;
                    }

                    case CONFIG_TYPE_FLOAT: {
                        if (a->maxf == FLT_MAX) {
                            mu_number(ctx, a->value, 0.1F);
                            *((float *) a->value) = max(a->minf, *((float *) a->value));
                        } else {
                            float_slider(ctx, a);
                        }

                        break;
                    }
                }

                if (a->help != NULL) {
                    mu_push_id(ctx, &a->value, sizeof(a->value));

                    if (mu_begin_popup(ctx, "Help")) {
                        mu_layout_row(ctx, 1, (int[]) {ctx->text_width(ctx->style->font, a->help, 0)}, 0);
                        mu_text(ctx, a->help);
                        mu_end_popup(ctx);
                    }

                    if (mu_button(ctx, "?"))
                        mu_open_popup(ctx, "Help");

                    mu_pop_id(ctx);
                } else {
                    mu_layout_next(ctx);
                }
            }
        }

        mu_layout_row(ctx, 3, (int[]) {0.25F * width, 0.50F * width, -1}, 0);
        mu_layout_next(ctx);

        if (mu_button(ctx, "Apply changes")) {
            memcpy(&settings, &settings_tmp, sizeof(Options));
            window_fromsettings();
            sound_volume(settings.volume / 10.0F);
            config_save();
        }

        if (mu_header_ex(ctx, "Help", MU_OPT_EXPANDED)) {
            mu_layout_row(ctx, 1, (int[]) {-1}, -1);
            mu_text(ctx,
                    "To edit a value directly, [SHIFT]+LMB on its container to change it using the keyboard. You can "
                    "also drag on a container to modify its value relative to its current one.\n\nWhen finished click "
                    "[Apply changes] so that your settings are not lost.");
        }

        mu_end_panel(ctx);

        mu_end_window(ctx);
    }
}

static void hud_settings_keyboard(int key, int action, int mods, int internal) {
    UNUSED(mods); UNUSED(internal);

    if (action == WINDOW_PRESS && key == WINDOW_KEY_ESCAPE)
        if (!map_empty()) hud_change(&hud_ingame);
}

static void hud_settings_touch(void * finger, int action, float x, float y, float dx, float dy) {
    UNUSED(finger); UNUSED(action); UNUSED(dx); UNUSED(dy);

    window_setmouseloc(x, y);
}

HUD hud_settings = {
    hud_settings_init,
    NULL,
    hud_settings_render,
    hud_settings_keyboard,
    NULL,
    NULL,
    NULL,
    hud_settings_touch,
    NULL,
    NULL,
    hud_ui_images,
    0,
    0,
    NULL,
};
