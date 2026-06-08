#include "core/hud.h"
#include "util/log.h"
#include "wayland/listeners/global.h"
#include <wayland-client-core.h>

int main(int argc, char **argv) {
    // Hide compile warning
    (void)!argc;
    (void)!argv;

    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        ERROR("failed to init wayland display.");
        return -1;
    }

    INFO("Success: Connected to the Wayland display successfully!");
    struct wl_registry *registry = wl_display_get_registry(display);
    if (!registry) {
        ERROR("Failed to get display registry.");
        return -1;
    }

    INFO("Success: Connected to the display registry successfully!");
    struct hud_state state;
    if (hud_state_init(&state, registry, display) < 0) {
        ERROR("failed to init hud state.");
        return -1;
    }

    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(display); // Roundtrip 1: Gets globals (compositor, etc..)

    hud_state_active(&state);

    for (;;) {}

    if (hud_state_destroy(&state) < 0)
        ERROR("failed to destory hud state");

    return 0;
}
