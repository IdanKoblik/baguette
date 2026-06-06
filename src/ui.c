#define _GNU_SOURCE

#include "ui.h"

#include <unistd.h>
#include <sys/mman.h>
#include "log.h"

#define HUD_PADDING 12

int init_surface(struct hud_state *state) {
    if (!state)
        return -1;

    INFO("creating new surface.");
    struct wl_surface *surface = wl_compositor_create_surface(state->compositor);
    if (!surface) {
        ERROR("failed to create surface.");
        return  -1;
    }
    state->surface = surface;

    INFO("creating new layer surface.");
    struct zwlr_layer_surface_v1 *layer_surface = zwlr_layer_shell_v1_get_layer_surface(state->layer_shell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_TOP, NAMESPACE);
    if (!layer_surface) {
        ERROR("failed to create layer surface.");
        return  -1;
    }

    state->layer_surface = layer_surface;
    zwlr_layer_surface_v1_set_anchor(layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);

    zwlr_layer_surface_v1_set_size(layer_surface, 0, HEIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, HEIGHT);

    wl_surface_commit(surface);
    return 0;
}

int init_buffer(struct hud_state *state) {
    if (!state)
        return -1;

    INFO("creating new layer buffer.");
    int fd = memfd_create("", 0);
    if (fd < 0) {
        ERROR("failed to init annon file.");
        return -1;
    }

    // The surface is sized in logical pixels, but on a scaled output we must
    // allocate the buffer at device resolution and tell the compositor the
    // scale. Otherwise a 1x buffer gets upscaled and the text looks blurry.
    int scale = state->scale > 0 ? state->scale : 1;
    int dev_width = state->width * scale;
    int dev_height = state->height * scale;

    int stride = dev_width * sizeof(uint32_t);
    int size = dev_height * stride;
    if (ftruncate(fd, size) < 0) {
        ERROR("failed to truncate annon file.");
        return -1;
    }

    uint32_t *pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pixels == MAP_FAILED) {
        ERROR("failed to map shm buffer.");
        return -1;
    }
    state->pixels = pixels;

    struct wl_shm_pool *shm_pool = wl_shm_create_pool(state->shm, fd, size);
    if (!shm_pool) {
        ERROR("failed to init shm pool.");
        return -1;
    }
    state->shm_pool = shm_pool;

    struct wl_buffer *buffer = wl_shm_pool_create_buffer(shm_pool, 0, dev_width, dev_height, stride, WL_SHM_FORMAT_ARGB8888);
    if (!buffer) {
        ERROR("failed to create buffer.");
        return -1;
    }
    state->buffer = buffer;

    // Wrap the shm pixels in a Cairo surface so we can draw straight into the
    // buffer. CAIRO_FORMAT_ARGB32 is native-endian premultiplied 0xAARRGGBB,
    // which matches WL_SHM_FORMAT_ARGB8888.
    state->cairo_surface = cairo_image_surface_create_for_data( (unsigned char *)pixels, CAIRO_FORMAT_ARGB32, dev_width, dev_height, stride);
    if (cairo_surface_status(state->cairo_surface) != CAIRO_STATUS_SUCCESS) {
        ERROR("failed to create cairo surface.");
        return -1;
    }

    state->cairo = cairo_create(state->cairo_surface);
    if (cairo_status(state->cairo) != CAIRO_STATUS_SUCCESS) {
        ERROR("failed to create cairo context.");
        return -1;
    }

    // Draw using logical coordinates; Cairo maps them onto device pixels.
    cairo_scale(state->cairo, scale, scale);

    wl_surface_set_buffer_scale(state->surface, scale);
    wl_surface_attach(state->surface, buffer, 0, 0);
    wl_surface_damage_buffer(state->surface, 0, 0, dev_width, dev_height);
    wl_surface_commit(state->surface);

    return 0;
}

static void draw_section(cairo_t *cr, struct hud_state *state, const char *text, double x) {
    if (!text || !*text)
        return;

    cairo_text_extents_t te;
    cairo_text_extents(cr, text, &te);
    double y = (state->height - te.height) / 2 - te.y_bearing;

    cairo_move_to(cr, x - te.x_bearing, y);
    cairo_show_text(cr, text);
}

void hud_draw(struct hud_state *state, const char *left, const char *center, const char *right) {
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
    draw_section(cr, state, left, HUD_PADDING);

    // Center: ink box centred on the bar's width.
    if (center && *center) {
        cairo_text_extents_t te;
        cairo_text_extents(cr, center, &te);
        draw_section(cr, state, center, (state->width - te.width) / 2);
    }

    // Right: anchored to the right edge with padding.
    if (right && *right) {
        cairo_text_extents_t te;
        cairo_text_extents(cr, right, &te);
        draw_section(cr, state, right, state->width - HUD_PADDING - te.width);
    }

    cairo_surface_flush(state->cairo_surface);
}
