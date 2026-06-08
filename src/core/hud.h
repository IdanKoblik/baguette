#pragma once

#include <wayland-client-core.h>
#include <wayland-client.h>
#include "../wayland/protocols/wlr-layer-shell-unstable-v1-protocol.h"

#define HEIGHT 40 // px
#define NAMESPACE "IdanK/Baguette"

struct hud_state {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_registry *registry;

    struct zwlr_layer_shell_v1 *layer_shell;

    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;
};

int hud_state_init(struct hud_state *state, struct wl_registry *registry, struct wl_display *display);
int hud_state_active(struct hud_state *state);
int hud_state_destroy(struct hud_state *state);
