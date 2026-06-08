#pragma once

#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "../wayland/protocols/wlr-layer-shell-unstable-v1-protocol.h"

#define NAMESPACE "IdanK/Baguette"

struct hud_state {
    struct wl_compositor *compositor;
    struct wl_registry *registry;
};

int hud_state_init(struct hud_state *state, struct wl_registry *registry);
int hud_state_destroy(struct hud_state *state);
