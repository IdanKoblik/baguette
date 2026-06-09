#include "global.h"
#include "../../core/hud.h"
#include "../../util/log.h"
#include "../protocols/wlr-layer-shell-unstable-v1-protocol.h"
#include <wayland-client-protocol.h>

struct wl_registry_listener registry_listener = {.global = registry_handle_global};

void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                            const char *interface, uint32_t version) {
    if (!data) {
        ERROR("provided data is null.");
        return;
    }

    struct hud_state *state = (struct hud_state *)data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        DEBUG("loading compositor(interface=%s), name: %d (version: %d)", interface, name, version);
        // Bind v6+ when available so wl_surface emits preferred_buffer_scale,
        // which is how niri (and wlroots) advertise HiDPI scale.
        uint32_t ver = version < 6 ? version : 6;
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, ver);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        DEBUG("loading layer_shell_v1(interface=%s), name: %d (version: %d)", interface, name,
              version);
        state->layer_shell =
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        DEBUG("loading shm(interface=%s), name: %d (version: %d)", interface, name, version);
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
}
