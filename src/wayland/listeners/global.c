#include "global.h"
#include "../../util/log.h"
#include "../../core/hud.h"
#include "../protocols/wlr-layer-shell-unstable-v1-protocol.h"
#include <wayland-client-protocol.h>

struct wl_registry_listener registry_listener = {
    .global = registry_handle_global
};

void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (!data) {
        ERROR("provided data is null.");
        return;
    }

    struct hud_state *state = (struct hud_state *)data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        DEBUG("loading compositor(interface=%s), name: %d (version: %d)", interface, name, version);
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        DEBUG("loading layer_shell_v1(interface=%s), name: %d (version: %d)", interface, name, version);
        state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version);
    }
}
