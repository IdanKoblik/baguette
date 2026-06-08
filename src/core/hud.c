#include "hud.h"
#include "../util/log.h"
#include "buffer.h"
#include "surface.h"
#include <sys/mman.h>
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

    memset(state, 0, sizeof(*state));
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

    if (init_buffer(state) < 0) {
        ERROR("Cannot init hud buffer.");
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
    if (state->cairo)
        cairo_destroy(state->cairo);

    if (state->cairo_surface)
        cairo_surface_destroy(state->cairo_surface);

    if (state->pixels) {
        int scale = state->scale > 0 ? state->scale : 1;
        munmap(state->pixels, (state->width * scale) * (state->height * scale) * sizeof(uint32_t));
    }

    if (state->layer_shell)
        zwlr_layer_shell_v1_destroy(state->layer_shell);

    if (state->surface)
        wl_surface_destroy(state->surface);

    if (state->layer_surface)
        zwlr_layer_surface_v1_destroy(state->layer_surface);

    if (state->shm)
        wl_shm_destroy(state->shm);

    if (state->shm_pool)
        wl_shm_pool_destroy(state->shm_pool);

    if (state->compositor)
        wl_compositor_destroy(state->compositor);

    if (state->registry)
        wl_registry_destroy(state->registry);

    if (state->display)
        wl_display_disconnect(state->display);

    return 0;
}
