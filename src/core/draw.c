#include "draw.h"
#include "../util/log.h"
#include <cairo/cairo.h>

static void draw_section(struct hud_state *state, const char *text, double x) {
    if (!text || !*text)
        return;

    cairo_text_extents_t te;
    cairo_text_extents(state->cairo, text, &te);
    double y = (state->height - te.height) / 2 - te.y_bearing;

    cairo_move_to(state->cairo, x - te.x_bearing, y);
    cairo_show_text(state->cairo, text);
}

void draw_hud(struct hud_state *state, const char *left, const char *center, const char *right) {
    if (!state) {
        ERROR("cannot draw hud, hud state not found.");
        return;
    }

    cairo_t *cr = state->cairo;

    // Background color.
    cairo_set_source_rgb(cr, 0.063, 0.063, 0.063); // TODO config
    cairo_paint(cr);

    cairo_set_source_rgb(cr, 0.54, 0.66, 0.92); // TODO config
    cairo_select_font_face(cr, "JetbrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 13); // TODO config

    cairo_font_options_t *fo = cairo_font_options_create();
    cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_GRAY);
    cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
    cairo_font_options_set_hint_metrics(fo, CAIRO_HINT_METRICS_ON);
    cairo_set_font_options(cr, fo);
    cairo_font_options_destroy(fo);

    // Left: anchored to the left edge with padding.
    draw_section(state, left, HUD_PADDING);

    // Center: ink box centred on the bar's width.
    if (center && *center) {
        cairo_text_extents_t te;
        cairo_text_extents(cr, center, &te);
        draw_section(state, center, (state->width - te.width) / 2);
    }

    // Right: anchored to the right edge with padding.
    if (right && *right) {
        cairo_text_extents_t te;
        cairo_text_extents(cr, right, &te);
        draw_section(state, right, state->width - HUD_PADDING - te.width);
    }

    cairo_surface_flush(state->cairo_surface);
}
