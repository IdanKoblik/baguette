#include "draw.h"
#include "../util/log.h"
#include <cairo/cairo.h>
#include <math.h>
#include <string.h>

// Per-section "pill" geometry, mirroring the waybar modules:
// (waybar modules: padding 0.5rem 1rem, margin 5px 0, border-radius 5px)
#define PILL_RADIUS 5
#define PILL_VMARGIN 3 // top/bottom gap from the bar edges; smaller -> taller pill
#define PILL_PAD_X 14  // horizontal padding inside the pill (slim, a touch under waybar's 1rem)

enum hud_align { HUD_LEFT, HUD_CENTER, HUD_RIGHT };

static void rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    double deg = M_PI / 180.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -90 * deg,   0 * deg);
    cairo_arc(cr, x + w - r, y + h - r, r,   0 * deg,  90 * deg);
    cairo_arc(cr, x + r,     y + h - r, r,  90 * deg, 180 * deg);
    cairo_arc(cr, x + r,     y + r,     r, 180 * deg, 270 * deg);
    cairo_close_path(cr);
}

// One continuous background pill spanning the whole bar (HUD_STYLE_FULL).
static void draw_full_background(struct hud_state *state) {
    cairo_t *cr = state->cairo;
    double x = HUD_PADDING;
    double y = PILL_VMARGIN;
    double w = state->width - 2 * HUD_PADDING;
    double h = state->height - 2 * PILL_VMARGIN;

    cairo_set_source_rgb(cr, 0.063, 0.063, 0.063); // #101010, TODO config
    rounded_rect(cr, x, y, w, h, PILL_RADIUS);
    cairo_fill(cr);
}

// Draw one section. With pill=true it gets its own floating background
// (HUD_STYLE_SEPARATED); with pill=false only the text is drawn, on top of the
// shared full-width background (HUD_STYLE_FULL).
// dr/dg/db is the default colour, used for any span that carries no %{#rrggbb}
// tag (span->has_color == false).
static void draw_section(struct hud_state *state, const struct fmt_section *sec, enum hud_align align, int pill, double dr, double dg, double db) {
    const char *text = sec->text;
    if (!text || !*text)
        return;

    cairo_t *cr = state->cairo;

    // Box geometry is sized from the whole section's ink; the per-span colours
    // only change how the runs are painted, not where they sit.
    cairo_text_extents_t te;
    cairo_text_extents(cr, text, &te);

    // Pill wraps the text ink plus horizontal padding, vertically inset from
    // the bar edges so it reads as a floating module like waybar. All geometry
    // is rounded to whole pixels: drawing on fractional coordinates makes Cairo
    // antialias glyph and pill edges across pixel boundaries, which reads as
    // blurry/heavy text. Snapping to the grid keeps it crisp like pango/waybar.
    double box_w = round(te.width + 2 * PILL_PAD_X);
    double box_y = PILL_VMARGIN;
    double box_h = state->height - 2 * PILL_VMARGIN;
    double box_x;
    switch (align) {
        case HUD_LEFT:   box_x = HUD_PADDING; break;
        case HUD_CENTER: box_x = round((state->width - box_w) / 2); break;
        case HUD_RIGHT:  box_x = state->width - HUD_PADDING - box_w; break;
        default:         box_x = HUD_PADDING; break;
    }

    // Per-section pill background (#101010), only in separated mode.
    if (pill) {
        cairo_set_source_rgb(cr, 0.063, 0.063, 0.063); // TODO config
        rounded_rect(cr, box_x, box_y, box_w, box_h, PILL_RADIUS);
        cairo_fill(cr);
    }

    // Text, centred vertically in the bar and padded inside the pill. The pen
    // origin is snapped to whole pixels so glyphs land on the grid. Each colour
    // run is shown in turn; cairo_show_text advances the current point, so the
    // runs flow left-to-right with no per-run repositioning.
    double text_x = round(box_x + PILL_PAD_X - te.x_bearing);
    double text_y = round((state->height - te.height) / 2 - te.y_bearing);
    cairo_move_to(cr, text_x, text_y);
    for (size_t i = 0; i < sec->nspans; i++) {
        const struct fmt_span *sp = &sec->spans[i];
        if (sp->len == 0)
            continue;

        double tr = dr, tg = dg, tb = db;
        if (sp->has_color) {
            tr = ((sp->color >> 16) & 0xFF) / 255.0;
            tg = ((sp->color >>  8) & 0xFF) / 255.0;
            tb = ( sp->color        & 0xFF) / 255.0;
        }

        char seg[FMT_MAX_TEXT];
        memcpy(seg, sec->text + sp->start, sp->len);
        seg[sp->len] = '\0';

        cairo_set_source_rgb(cr, tr, tg, tb);
        cairo_show_text(cr, seg);
    }
}

void draw_hud(struct hud_state *state, const struct hud_info *info) {
    if (!state) {
        ERROR("cannot draw hud, hud state not found.");
        return;
    }

    if (!info) {
        ERROR("cannot draw hud, hud info not found.");
        return;
    }

    cairo_t *cr = state->cairo;

    // Transparent background (waybar: background: transparent). SOURCE operator
    // overwrites the previous frame's pixels rather than blending over them.
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);

    cairo_select_font_face(cr, "JetbrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 14); // TODO config

    cairo_font_options_t *fo = cairo_font_options_create();
    cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_GRAY);
    cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
    cairo_font_options_set_hint_metrics(fo, CAIRO_HINT_METRICS_ON);
    cairo_set_font_options(cr, fo);
    cairo_font_options_destroy(fo);

    // Full mode paints a single background spanning the bar; separated mode
    // gives each section its own pill (drawn inside draw_section).
    int pill = (state->style != HUD_STYLE_FULL);
    if (!pill)
        draw_full_background(state);

    draw_section(state, &info->left,   HUD_LEFT,   pill, 0.729, 0.733, 0.945); // #babbf1
    draw_section(state, &info->center, HUD_CENTER, pill, 0.549, 0.667, 0.933); // #8caaee
    draw_section(state, &info->right,  HUD_RIGHT,  pill, 0.776, 0.816, 0.961); // #c6d0f5

    cairo_surface_flush(state->cairo_surface);
}
