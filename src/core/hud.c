#include "hud.h"
#include "../util/log.h"
#include "surface.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

int hud_state_init(struct hud_state *state, struct wl_registry *registry, struct wl_display *display) {
    if (!state) {
        ERROR("Cannot init hud state -> null.");
        return -1;
    }

    if (!registry) {
        ERROR("Cannot init hud_state, registry -> null.");
        return -1;
    }

    if (!display) {
        ERROR("Cannot init hud_state, display -> null.");
        return -1;
    }

    //memset(state, 0, sizeof(*state));
    state->width = 0;
    state->height = 0;
    state->display = display;
    state->registry = registry;

    return 0;
}

int hud_state_active(struct hud_state *state) {
    if (!state) {
        ERROR("Cannot activate hud state -> null.");
        return -1;
    }

    if (init_surface(state) < 0) {
        ERROR("Cannot init hud surface.");
        return -1;
    }

    wl_display_roundtrip(state->display);
    return 0;
}

int hud_state_destroy(struct hud_state *state) {
    if (!state) {
        ERROR("Cannot destroy hud state -> null.");
        return -1;
    }

    INFO("Cleaning up hud state.");
    if (state->compositor)
        wl_compositor_destroy(state->compositor);

    if (state->registry)
        wl_registry_destroy(state->registry);

    if (state->display)
        wl_display_disconnect(state->display);

    return 0;
}
