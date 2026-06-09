#include "surface.h"
#include "../util/log.h"
#include "../wayland/listeners/layer_surface.h"
#include "../wayland/protocols/wlr-layer-shell-unstable-v1-protocol.h"

// The compositor reports HiDPI scale via wl_surface.preferred_buffer_scale.
static void surface_handle_preferred_buffer_scale(void *data, struct wl_surface *surface,
                                                  int32_t factor) {
    (void)surface;
    struct hud_state *state = (struct hud_state *)data;
    if (factor > 0)
        state->scale = factor;
    DEBUG("preferred buffer scale: %d", factor);
}

static void surface_handle_enter(void *data, struct wl_surface *surface, struct wl_output *output) {
    (void)data;
    (void)surface;
    (void)output;
}

static void surface_handle_leave(void *data, struct wl_surface *surface, struct wl_output *output) {
    (void)data;
    (void)surface;
    (void)output;
}

static void surface_handle_preferred_buffer_transform(void *data, struct wl_surface *surface,
                                                      uint32_t transform) {
    (void)data;
    (void)surface;
    (void)transform;
}

static const struct wl_surface_listener surface_listener = {
    .enter = surface_handle_enter,
    .leave = surface_handle_leave,
    .preferred_buffer_scale = surface_handle_preferred_buffer_scale,
    .preferred_buffer_transform = surface_handle_preferred_buffer_transform,
};

int init_surface(struct hud_state *state) {
    if (!state) {
        ERROR("cannot init surface, hud state is null.");
        return -1;
    }

    INFO("creating new surface.");
    struct wl_surface *surface = wl_compositor_create_surface(state->compositor);
    if (!surface) {
        ERROR("failed to create surface.");
        return -1;
    }
    state->surface = surface;
    wl_surface_add_listener(surface, &surface_listener, state);

    INFO("creating new layer surface.");
    struct zwlr_layer_surface_v1 *layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        state->layer_shell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_TOP, NAMESPACE);
    if (!layer_surface) {
        ERROR("failed to create layer surface.");
        return -1;
    }

    state->layer_surface = layer_surface;
    zwlr_layer_surface_v1_add_listener(state->layer_surface, &layer_surface_listener, state);

    zwlr_layer_surface_v1_set_anchor(layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                                        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                                        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);

    zwlr_layer_surface_v1_set_size(layer_surface, 0, HEIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, HEIGHT);

    wl_surface_commit(surface);

    return 0;
}
