#pragma once

#include <wayland-client.h>

#define NAMESPACE "IdanK/Baguette"

struct hud_state {
    struct wl_compositor *compositor;
    struct wl_registry *registry;
};

int hud_state_init(struct hud_state *state, struct wl_registry *registry);
int hud_state_destroy(struct hud_state *state);
