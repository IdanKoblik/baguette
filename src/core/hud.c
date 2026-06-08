#include "hud.h"
#include "../util/log.h"
#include <wayland-client-protocol.h>

int hud_state_init(struct hud_state *state, struct wl_registry *registry) {
    if (!state) {
        ERROR("Cannot init hud state -> null.");
        return -1;
    }

    if (!registry) {
        ERROR("Cannot init hud_state, registry -> null.");
        return -1;
    }

    memset(state, 0, sizeof(*state));
    state->registry = registry;
    state->compositor = NULL;

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

    return 0;
}
