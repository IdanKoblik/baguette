#pragma once

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-protocol.h"

#define HEIGHT 40
#define NAMESPACE "IdanK/Baguette"

struct hud_state {
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layer_shell;

    struct wl_shm *shm;
    struct wl_shm_pool *shm_pool;
    struct wl_buffer *buffer;

    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;

    uint32_t *pixels;

    uint32_t width;
    uint32_t height;
};

void hud_state_destroy(struct hud_state *state);
