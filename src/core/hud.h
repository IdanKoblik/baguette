#pragma once

#include <unistd.h>
#include <poll.h>
#include <cairo/cairo.h>
#include <linux/limits.h>
#include <wayland-client-core.h>
#include <wayland-client.h>
#include "../wayland/protocols/wlr-layer-shell-unstable-v1-protocol.h"
#include "format.h"

#define HEIGHT 40 // px
#define NAMESPACE "IdanK/Baguette"

enum hud_style {
    HUD_STYLE_SEPARATED,
    HUD_STYLE_FULL,
};

// Each section carries its decoded text plus an optional %{#rrggbb} colour.
struct hud_info {
    struct fmt_section left;
    struct fmt_section center;
    struct fmt_section right;
};

struct hud_state {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_registry *registry;

    struct zwlr_layer_shell_v1 *layer_shell;

    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;

    struct wl_buffer *buffer;

    struct wl_shm *shm;
    struct wl_shm_pool *shm_pool;

    int height;
    int width;

    int32_t scale;          // scale advertised by the compositor (0 until known)
    int32_t rendered_scale; // scale the current buffer was actually built at
    size_t map_size;
    uint32_t *pixels;

    cairo_surface_t *cairo_surface;
    cairo_t *cairo;

    enum hud_style style;
    struct hud_info *info;
};

int hud_state_init(struct hud_state *state, struct wl_registry *registry, struct wl_display *display);
int hud_state_active(struct hud_state *state);
void hud_info_process(struct hud_info *info, struct pollfd *stdin_fd);
int hud_state_destroy(struct hud_state *state);
