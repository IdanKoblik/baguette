#include "core/hud.h"
#include "util/log.h"
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
    if (hud_state_init(&state, registry) < 0) {
        ERROR("failed to init hud state.");
        return -1;
    }

    if (hud_state_destroy(&state) < 0)
        ERROR("failed to destory hud state");

    wl_display_disconnect(display);
    return 0;
}
