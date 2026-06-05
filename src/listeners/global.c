#include <string.h>
#include <wayland-client-protocol.h>
#include "global.h"
#include "../hud.h"
#include "../log.h"
#include "../wlr-layer-shell-unstable-v1-protocol.h"

static void output_handle_scale(void *data, struct wl_output *wl_output, int32_t factor) {
    (void)wl_output;
    struct hud_state *state = (struct hud_state *)data;
    if (factor > 0)
        state->scale = factor;
    DEBUG("output scale: %d", factor);
}

static void output_handle_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
                                   int32_t physical_width, int32_t physical_height, int32_t subpixel,
                                   const char *make, const char *model, int32_t transform) {
    (void)data; (void)wl_output; (void)x; (void)y; (void)physical_width;
    (void)physical_height; (void)subpixel; (void)make; (void)model; (void)transform;
}

static void output_handle_mode(void *data, struct wl_output *wl_output, uint32_t flags,
                               int32_t width, int32_t height, int32_t refresh) {
    (void)data; (void)wl_output; (void)flags; (void)width; (void)height; (void)refresh;
}

static void output_handle_done(void *data, struct wl_output *wl_output) {
    (void)data; (void)wl_output;
}

static const struct wl_output_listener output_listener = {
    .geometry = output_handle_geometry,
    .mode = output_handle_mode,
    .done = output_handle_done,
    .scale = output_handle_scale,
};

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
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        DEBUG("loading shm(interface=%s), name: %d (version: %d)", interface, name, version);
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        // Bind at v2 for the scale event; v3+ name/description events are
        // left unhandled by design (never delivered at this version).
        uint32_t bind_version = version < 2 ? version : 2;
        DEBUG("loading output(interface=%s), name: %d (version: %d)", interface, name, bind_version);
        state->output = wl_registry_bind(registry, name, &wl_output_interface, bind_version);
        wl_output_add_listener(state->output, &output_listener, state);
    }
}
