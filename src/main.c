#include <string.h>
#include <unistd.h>
#include <time.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "hud.h"
#include "listeners/layer_surface.h"
#include "listeners/global.h"
#include "log.h"
#include "ui.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

static struct wl_registry_listener registry_listener = {
    .global = registry_handle_global
};

static struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_handle
};

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        ERROR("Could not connect to a Wayland display.");
        return 1;
    }

    INFO("Success: Connected to the Wayland display successfully!");
    struct wl_registry *registry = wl_display_get_registry(display);
    if (!registry) {
        ERROR("Failed to get display registry.");
        return 1;
    }

    struct hud_state state;
    memset(&state, 0, sizeof(state));

    state.width = 0;
    state.height = 0;
    state.scale = 1;

    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_roundtrip(display);

    if (init_surface(&state) < 0) {
        ERROR("failed to init hud surface.");
        return 1;
    }

    zwlr_layer_surface_v1_add_listener(state.layer_surface, &layer_surface_listener, &state);

    wl_display_roundtrip(display);

    if (init_buffer(&state) < 0) {
        ERROR("failed to init hud buffer.");
        return 1;
    }

    wl_display_roundtrip(display);

    for (;;) {
        time_t now = time(NULL);
        struct tm tm;
        localtime_r(&now, &tm);

        char center[64];
        strftime(center, sizeof(center), "%H:%M:%S", &tm);

        char right[128];
        strftime(right, sizeof(right), "%a %d %b", &tm);

        hud_draw(&state, "baguette", center, right);

        wl_surface_attach(state.surface, state.buffer, 0, 0);
        wl_surface_damage_buffer(state.surface, 0, 0, state.width, state.height);
        wl_surface_commit(state.surface);
        wl_display_flush(display);

        sleep(1);
    }

    hud_state_destroy(&state);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);
    return 0;
}
