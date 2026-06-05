#include "hud.h"
#include "log.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

int init_hud_state(struct hud_state *state) {
    if (!state)
        return -1;

    INFO("creating new surface.");
    struct wl_surface *surface = wl_compositor_create_surface(state->compositor);
    if (!surface) {
        ERROR("failed to create surface.");
        return 1;
    }

    INFO("creating new layer surface.");
    struct zwlr_layer_surface_v1 *layer_surface = zwlr_layer_shell_v1_get_layer_surface(state->layer_shell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_TOP, NAMESPACE);
    if (!layer_surface) {
        ERROR("failed to create layer surface.");
        return 1;
    }

    zwlr_layer_surface_v1_set_anchor(layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);

    zwlr_layer_surface_v1_set_size(layer_surface, 0, HEIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, HEIGHT);

    wl_surface_commit(surface);

    return 0;
}

void hud_state_destroy(struct hud_state *state) {
    if (!state)
        return;

    INFO("cleaning up hud_state.");
    if (state->compositor)
        wl_compositor_destroy(state->compositor);

    if (state->layer_shell)
        zwlr_layer_shell_v1_destroy(state->layer_shell);

    if (state->surface)
        wl_surface_destroy(state->surface);

    if (state->layer_surface)
        zwlr_layer_surface_v1_destroy(state->layer_surface);
}
