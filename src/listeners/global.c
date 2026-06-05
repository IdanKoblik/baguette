#include <string.h>
#include <wayland-client-protocol.h>
#include "global.h"
#include "../hud.h"
#include "../log.h"
#include "../wlr-layer-shell-unstable-v1-protocol.h"

void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (!data) {
        ERROR("the data is null.");
        return;
    }

    // Remove comment for debugging (DO NOT KEEP IN PRODUCTION CODE)
    //LOG("%d, %s, %d", name, interface, version);

    struct hud_state *state = (struct hud_state *)data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        DEBUG("loading compistor(interface=%s), name: %d (version: %d)", interface, name, version);
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        DEBUG("loading layer_shell_v1(interface=%s), name: %d (version: %d)", interface, name, version);
        state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version);
    }
}
