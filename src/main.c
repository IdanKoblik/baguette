#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "hud.h"
#include "listeners/global.h"
#include "log.h"

static struct wl_registry_listener registry_listener = {
    .global = registry_handle_global
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
    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_roundtrip(display);

    if (init_hud_state(&state) < 0) {
        ERROR("failed to init hud state.");
        return 1;
    }

    wl_display_roundtrip(display);
    for (;;);

    hud_state_destroy(&state);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);
    return 0;
}
