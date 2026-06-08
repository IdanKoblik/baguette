#include "core/hud.h"
#include "util/log.h"
#include "wayland/listeners/global.h"
#include "core/draw.h"
#include <unistd.h>

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
    struct hud_state state = {0};
    if (hud_state_init(&state, registry, display) < 0) {
        ERROR("failed to init hud state.");
        return -1;
    }

    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(display); // Roundtrip 1: Gets globals (compositor, etc..)

    if (hud_state_active(&state) < 0) {
        ERROR("failed to activate hud.");
        return -1;
    }

    INFO("Success: Loaded hud successfully!");
    for (;;) {
        char temp[64];
        draw_hud(&state, "baguette", temp, temp);

        wl_surface_attach(state.surface, state.buffer, 0, 0);
        wl_surface_damage_buffer(state.surface, 0, 0, state.width, state.height);
        wl_surface_commit(state.surface);
        wl_display_flush(display);

        sleep(1);
    }

    if (hud_state_destroy(&state) < 0)
        ERROR("failed to destory hud state");

    return 0;
}
