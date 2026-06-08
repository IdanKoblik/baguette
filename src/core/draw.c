#include "draw.h"
#include "../util/log.h"
#include <cairo/cairo.h>

void draw_hud(struct hud_state *state, const char *left, const char *center, const char *right) {
    if (!state) {
        ERROR("cannot draw hud, hud state not found.");
        return;
    }

    cairo_t *cr = state->cairo;
    // Background color.
    cairo_set_source_rgb(cr, 0.063, 0.063, 0.063); // TODO config
    cairo_paint(cr);

    cairo_surface_flush(state->cairo_surface);
}
